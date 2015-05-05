// SYSEN 5200/5210 Project - Discrete Event Simulation
// Author: Carlos Patiño, cap273
// Group Partners: Jason Fitzgerald, Alexander Masetti
// Written in C++ using Microsoft Visual Studio Express 2013
//
// This program runs a discrete-event simulation of a manufacturing plant
// that takes as input 5 parts: piston, cylinder-cap, culinder, cylinder rod-end, 
// and a rod-end. These 5 parts are inspected separately and either passed or 
// rejected. They are then passed onto an assembly plant, then a coating plant,
// and finally an assembly-level inspection plant. Accepted parts leave the system.
// Rejected parts are sent to a re-work station, and from then are sent to back to
// the assembly plant.
//
// The relevant statistics from each simulation are stored in a CSV file.

//////////////////////////////////////////////////
///// Include statements, forward declarations ///
///////////// and other initializations //////////
//////////////////////////////////////////////////

#include <iostream>
#include <vector> // Package to use vectors
#include <string> // Package to handle strings
#include <random> // Package to handle rand, exponential distribution and normal distribution functions
#include <list> // Package to handle lists
#include <iomanip>    // std::setprecision
#include <fstream> // Package for streams to write to CSV file
// #include <sstream> // Package for string streams
#include <algorithm> // To use std::find
#include <map> // To use std::map
#include <queue>  // std::queue

#include "Event.h" // Include class Event

using namespace std;

// Forward declarations of functions
Event getNextEvent(list<Event>);
void printListOfEvents(list<Event>);

double exponentialDist(double);
double normalDist(double, double);

void resetAll(void);
void createAssemblyEntity(void);

void arrival(char);
void departureAssembly(double);
void departureCoating(double);
void departureReWork(double);

// Declare list of events (the Future Event list, FEL) and its iterator it
list<Event> listOfEvents;
list<Event>::iterator it = listOfEvents.begin();

// Declare simulation time variable.
double simulationTime;

// Declare a part ID variable. This is the next ID number that is to be associated with an Assembly entity.
double nextID;

// Declare a map of part IDs and simulation times when the part ID (i.e. the Assembly entity) was created.
// This map should only contain partIDs currently in the system. Initialize its iterator.
// Example: add partID = 5 with creation_time = 600 to the map: creationTimes[5] = 600;
// Example: access the creation time of partID = 5: creationTimes.find(5)->second;
// Example: delete a particular partID from the map: creationTimes.erase(creatinTimes.find ('e'));
map<double, double> creationTimes;
map<double, double>::iterator it_creationTime;

// Declare a list of part IDs of Assembly entities currently in the system that have undergone rework.
list<double> partIDsRework;
list<double>::iterator it_rework = partIDsRework.begin();

// Declare a queue of part IDs of Assembly entities currently in the Coating queue
// We need a queue because we need to use First In First Out
// Push to add a new elements to the queue, which are then popped out in the same order.
// Ex.: myqueue.front() accesses the next element
queue<double> coatingQueue;

// Declare a queue of part IDs of Assembly entities currently in the ReWork queue
queue<double> reWorkQueue;

// Declare a list of part IDs of Assembly entities currently in the Assembly station queue that are coming from the ReWork station
queue<double> assemblyStationQueue;

//////////////////////////////////////////////////
//             User initializations             //
// (Note that all variables declared before     //
//        main() are global variables           //
//////////////////////////////////////////////////

// Declare end of simulation time (mins)
double endSimulationTime = 1080;

// Specify number of simulations to run
int numSimulations = 10;

// Define ramp-up time. Only start recording statistics of interest after this ramp-up time.
double rampUpTime = 120;

// Calculate time of interest
double timeOfInterest = endSimulationTime - rampUpTime;

// Declare service times and interarrival time means. These can be changed by the user here.
double const interArr_RodEnd = 5;
double const interArr_Piston = 5;
double const interArr_CylinderCap = 5;
double const interArr_Cylinder = 5;
double const interArr_CylinderRodEnd = 5;

double const srvcTime_Assembly_Mean = 4;
double const srvcTime_Assembly_Stdev = 1;

double const srvcTime_Coating_Mean = 5;
double const srvcTime_Coating_Stdev = 3;

double const srvcTime_Rework_Mean = 10;
double const srvcTime_Rework_Stdev = 4;

// Define acceptance/rejection probabilities for each part.
double const accProb_RodEnd = 0.996;
double const accProb_Piston = 0.999;
double const accProb_CylinderCap = 1.0;
double const accProb_Cylinder = 0.999;
double const accProb_CylinderRodEnd = 0.998;

double const accProb_Assembly_NoRework = 0.868939;
double const accProb_Assembly_Rework = accProb_Assembly_NoRework*1.50;

// Define state of system
struct state_t {
	int numAssembly; // Total number of Assemblies in the entire system
	int numAssembly_preCoat; // Total number of Assemblies either in the coating station or in the coating queue
	int numAssembly_preReWork; // Total number of Assemblies either in the rework station or in the rework queue
	int numAssembly_preAssembly; // Total number of Assemblies that are either in the Assembly station or in the Assembly queue

	int numRodEnd; // Total number of Rod Ends in the entire system (only in the Rod End Receiving Station)
	int numPiston; // Total number of Pistons in the entire system (only in the Piston Receiving Station)
	int numCylinderCap; // Total number of Cylinder Caps in the entire system (only in the Cylinder Cap Receiving Station)
	int numCylinder; // Total number of Cylinders in the entire system(only in the Cylinder Receiving Station)
	int numCylinderRodEnd; // Total number of Cylinder Rod Ends in the entire system. (only in the Cylinder Rod End Receiving Station)

} systemState;

//Define and initialize statistics of interest
double totalAssembliesCreated; // An assembly is created when 5 parts are merged into a 'pre assembly entity'
double totalAssembliesDelivered; // An assembly is delivered when it leaves the system.

double totalTimeAssembliesInSystem; // Cumulative time that all assemblies that LEFT the system have spent in the system.
double cumAssemblies_Time_InSystem; // Cumulative sum of (total number of assemblies in the system * time)

double totalTimeAssemblyStationBusy; // Cumulative time that Assembly station has been busy
double totalTimeReWorkStationBusy; // Cumulative time that ReWork station has been busy

// Main function for simulation
int main()
{

	// Declare name of CSV file to which to output ALL results and open it.
	ofstream Simulation_Runs("Simulation_Runs.csv", ios::out);

	// Declare name of CSV file to which to output the statistics of interest at the end of each simulation
	ofstream Simulation_Results("Simulation_Results.csv", ios::out);

	// Make header for simulation results CSV
	Simulation_Results << "Simulation Number" << "," << "Assemblies Created" << "," << "Assemblies Delivered" << "," <<
		"Average Assembly Time in System" << "," << "Average Num Assemblies in System" << "," << "Prop. Assembly St. Busy" << "," <<
		"," << "Prop. Rework Busy" << endl;

	// Loop through all simulations
	for (int i = 0; i < numSimulations; ++i){

		// Place a header line in the CSV file before each simulation
		Simulation_Runs << "Simulation Number: " << i + 1 << endl;

		// Reset list of Events, simulation time, statistics of interest and state of the system
		// before a new simulation run.
		resetAll();

		// Schedule end of simulation event (order of insertion into list does not matter)
		listOfEvents.insert(it, Event('e', endSimulationTime));

		// Schedule also arrival events for all 5 parts
		listOfEvents.insert(it, Event('a', simulationTime + exponentialDist(interArr_RodEnd)));
		listOfEvents.insert(it, Event('b', simulationTime + exponentialDist(interArr_Piston)));
		listOfEvents.insert(it, Event('c', simulationTime + exponentialDist(interArr_CylinderCap)));
		listOfEvents.insert(it, Event('d', simulationTime + exponentialDist(interArr_Cylinder)));
		listOfEvents.insert(it, Event('f', simulationTime + exponentialDist(interArr_CylinderRodEnd)));

		//Finally, initialize a placeholder Event variable, placeholder char variable, and placeholder string for use in simulation
		Event nextEvent = Event();
		char eventType;
		string eventTypeStr;
		double eventPartID; // Holds the part ID associated with an assembly entity (and held in an event)

		// Make simulation header for simulation run SV
		Simulation_Runs << "Simulation Time, Event Type,  Pre Assembly, Pre Coating, Pre ReWork, Assemblies Created, Assemblies Delivered,"
			"Total Assembly Time in System" << "," << "cumAssemblies_Time_InSystem" << "," << "Event ID" << "," << "next ID" << endl;

		//////////////////////////////
		// Begin actual simulation //
		/////////////////////////////

		// Loop through each event in the simulation until the end of the simulation is reached.
		while (1) {

			// Get next event in list of events
			nextEvent = getNextEvent(listOfEvents);

			///////////////////////////////////
			// Update statistics of interest for interval //
			//////////////////////////////////

			// Calculate interval. Note that the variable 'simulationTime' still holds
			// the time of the PREVIOUS event (it has not been updated yet).
			double intervalTime = nextEvent.getTimeOfEvent() - simulationTime;

			// Only update if simulation time is greater than ramp-up time
			if (simulationTime > rampUpTime) {

				// Update cumulative sum of (assemblies in system * time)
				cumAssemblies_Time_InSystem = cumAssemblies_Time_InSystem +
					((systemState.numAssembly_preCoat + systemState.numAssembly_preCoat + systemState.numAssembly_preReWork)*intervalTime);

				// Update assembly station busy.
				if (systemState.numAssembly_preAssembly >= 1) {
					totalTimeAssemblyStationBusy = totalTimeAssemblyStationBusy + intervalTime;
				}

				// Update rework station busy
				if (systemState.numAssembly_preReWork >= 1) {
					totalTimeReWorkStationBusy = totalTimeReWorkStationBusy + intervalTime;
				}
			}

			// Update simulation time
			simulationTime = nextEvent.getTimeOfEvent();

			// Get what the event type is, plus a string describing the event type
			eventType = nextEvent.getEventType();
			eventTypeStr = nextEvent.eventTypeToString();

			// Get part ID from the event
			eventPartID = nextEvent.getPartID();

			//Delete event that just occurred from the list of events (!!)
			listOfEvents.remove(nextEvent);

			// For debugging purposes, print current FEL to the command window
			printListOfEvents(listOfEvents);

			// If end of simulation, exit while loop
			if (eventType == 'e') {

				break;
			}

			//*****************************************************************************
			// If simulation is not over, perform appropriate actions depending on what the event type is.
			//*****************************************************************************
			switch (eventType) {
			case 'a':
				arrival(eventType);
				break;
			case 'b':
				arrival(eventType);
				break;
			case 'c':
				arrival(eventType);
				break;
			case 'd':
				arrival(eventType);
				break;
			case 'f':
				arrival(eventType);
				break;

			case 'x':
				departureAssembly(eventPartID);
				break;
			case 'y':
				departureCoating(eventPartID);
				break;
			case 'z':
				departureReWork(eventPartID);
				break;
			default:
				cout << "Error, bad input, quitting\n";
				break;
			}			

			// Sequentially write simulation run output to CSV file.
			Simulation_Runs << setprecision(4) << simulationTime << "," << eventTypeStr << "," << systemState.numAssembly_preAssembly << "," <<
				systemState.numAssembly_preCoat << "," << systemState.numAssembly_preReWork << "," << totalAssembliesCreated << "," << 
				totalAssembliesDelivered << "," << totalTimeAssembliesInSystem << "," << cumAssemblies_Time_InSystem  << "," << eventPartID << 
				"," << nextID << endl;

			
		} // End of while-loop

		//At the end of each simulation, add a blank line to the CSV file.
		Simulation_Runs << endl;

		// Add results from each simulation to the 'results' CSV file
		Simulation_Results << i + 1 << "," << totalAssembliesCreated << "," << totalAssembliesDelivered << "," <<
			totalTimeAssembliesInSystem/timeOfInterest << "," << cumAssemblies_Time_InSystem/timeOfInterest << "," << 
			totalTimeAssemblyStationBusy/timeOfInterest << "," <<"," << totalTimeReWorkStationBusy/timeOfInterest << endl;

		// Print final results:

		std::cout << "----------------------" << endl;
		std::cout << "------------------------" << endl;

		/*
		cout << setprecision(4) << "Proportion of time server 1 is busy: " << totalTimeServer1Busy / simulationTime << endl;
		cout << setprecision(4) << "Proportion of time both servers are busy: " << totalTimeBothServerBusy / simulationTime << endl;
		cout << setprecision(4) << "Proportion of time there are 1 or more customers in the queue: " << totalTimeCustInQueue / simulationTime << endl;
		*/		

	} // end of for-loop

	// Close CSV file on which ALL simulation runs are stored
	Simulation_Runs.close();

	// Close CSV file on which all summary statistics are stored
	Simulation_Results.close();

	// Force command window to stay open.
	std::cin.get();
	std::cin.get();

	return 0; // End of main function
}

//////////////////////////////////////////////////
//               Helper functions               //
//////////////////////////////////////////////////

/* For debugging purposes */

// Returns the event that is next to occur (i.e. has the lowest ["closest"] scheduled time)
Event getNextEvent(list<Event> listOfEvents)
{
	// Initialize dummy Event.
	Event nextEvent('w', std::numeric_limits<double>::infinity());

	// For every Event in listOfEvents, check whether the timeOfEvent is smaller than current nextEvent. If it is, nextEvent
	// takes on current Event.
	for (list<Event>::iterator it = listOfEvents.begin(); it != listOfEvents.end(); it++)
	{
		if ((*it).getTimeOfEvent() < nextEvent.getTimeOfEvent())
			nextEvent = *it;
	}

	return nextEvent;
}

// Prints list of Events to the command window
void printListOfEvents(list<Event> listOfEvents)
{
	for (list<Event>::iterator it = listOfEvents.begin(); it != listOfEvents.end(); it++)
		std::cout << "Event type: " << (*it).getEventType() << ".\t Event time: " << (*it).getTimeOfEvent() << endl;

	std::cout << "----------------" << endl;
}

/* To produce samples from exponential and normal distributions */

// Return a sample from the exponential distribution after specifying the interarrival time mu.
// Three things with exponential distributions:
// Parameter lambda, or the average rate of occurence. How much an event happens in a period of time. (e.g. 0.5 calls in an hour)
// Mean, or the expected value of an exponentially distributed RV with parameter lambda. This is in units of time.
// Interarrival time, or the average time between events occuring. (e.g. every 2 hours, one call). This is identical to the mean.
double exponentialDist(double mu)
{
	// Initialize variables for exponential distribution
	std::random_device rd;
	std::exponential_distribution<> rng(1/mu); //Input must be parameter lambda, which is the average rage of occurrence
	std::mt19937 rnd_gen(rd());

	// Return sample from the exponential distribution with lambda = 1/mu
	return rng(rnd_gen);
}

// Return a sample from the normal distribution with mean 'mean' and standard deviation 'sigma'
double normalDist(double mean, double sigma)
{
	// Initialize variables for normal distribution
	std::random_device rd;
	std::normal_distribution<double> distribution(mean, sigma);
	std::mt19937 rnd_gen(rd());

	// Return sample from the normal distribution
	return distribution(rnd_gen);
}

/* Handle new simulations. */

// Reset list of Events, simulation time, statistics of interest and state of the system
// before a new simulation run.
void resetAll(void) {

	// Set simulation time to zero
	simulationTime = 0;

	// Clears all elements from the list of events
	listOfEvents.clear();

	// Clears all elements from the list of IDs currently being reworked.
	partIDsRework.clear();

	// Clears all elements in the map containing partIDs and creation times of corresponding assembly entities
	creationTimes.clear();

	// Clears all queues
	coatingQueue = queue<double>();
	reWorkQueue = queue<double>();
	assemblyStationQueue = queue<double>();

	// Reset next part ID for an Assembly entity
	// Remember that an ID of 0 is a tag for an invalid ID (i.e. no ID)
	nextID = 1;

	// Reset statistics of interest
	totalAssembliesCreated = 0; // An assembly is created when 5 parts are merged into a 'pre assembly entity'
	totalAssembliesDelivered = 0; // An assembly is delivered when it leaves the system.
	totalTimeAssembliesInSystem = 0; // Cumulative time that all assemblies that LEFT the system have spent in the system.
	cumAssemblies_Time_InSystem = 0; // Cumulative sum of (total number of assemblies in the system * time)
	totalTimeAssemblyStationBusy = 0; // Cumulative time that Assembly station has been busy
	totalTimeReWorkStationBusy = 0; // Cumulative time that ReWork station has been busy

	// Reset state of system
	systemState.numAssembly = 0;
	systemState.numAssembly_preAssembly = 0;
	systemState.numAssembly_preCoat = 0;
	systemState.numAssembly_preReWork = 0;

	systemState.numCylinder = 0;
	systemState.numCylinderCap = 0;
	systemState.numCylinderRodEnd = 0;
	systemState.numPiston = 0;
	systemState.numRodEnd = 0;

}

////////////////////////////////////////////////////////////////
/* To handle changes in the system state when an event occurs */
////////////////////////////////////////////////////////////////

// If there are at least 1 part of each in the system, create new assembly entity awaiting to 
// go to the Assembly Station, tag it in the listOfEvents, and reduce the number of other 
// parts in the system by 1.
void createAssemblyEntity(void) {

	// If there is at least one part of each.
	if ((systemState.numRodEnd * systemState.numPiston * systemState.numCylinderRodEnd *
		systemState.numCylinderCap * systemState.numCylinder) > 0) {

		// Increment total number of assembly entities created if simulation time is beyond ramp-up time
		if (simulationTime > rampUpTime)
			totalAssembliesCreated++;

		// Reduce the number of each part in the system by 1
		systemState.numRodEnd--;
		systemState.numPiston--;
		systemState.numCylinderRodEnd--;
		systemState.numCylinderCap--;
		systemState.numCylinder--;

		// Increment the number of assembly entities awaiting to go to the assembly station by 1.
		systemState.numAssembly_preAssembly++;

		// Add part ID of this new assembly to the Assembly station Queue
		assemblyStationQueue.push(nextID);

		// Add this Assembly to the map containing creation times for assembly entities based on their part numbers
		creationTimes[nextID] = simulationTime;

		// Increment the nextID variable by one for use for the next Assembly entity
		nextID++;

		// If there is only one preAssembly entity, schedule a departure event from the Assembly station
		// Also tag the assembly part with a part ID
		if (systemState.numAssembly_preAssembly == 1) {

			listOfEvents.insert(it, Event('x', 
				simulationTime + normalDist(srvcTime_Assembly_Mean, srvcTime_Assembly_Stdev), assemblyStationQueue.front()));

			// Remove an assembly entity ID from the queue
			assemblyStationQueue.pop();
	
		}
	}
}

// Execute system state changes when an arrival of a Rod End occurs
void arrival(char partType) {

	double prob = rand() % 100; // prob in the range 0 to 99. Used to determine if part was accepted or rejected.
	bool partAccepted = false; // true if the part in question was accepted, false otherwise

	// Increment the corresponding number of parts in the system
	switch (partType) {
	case 'a':
		if (prob < (accProb_RodEnd*100)) {
			// Increment the number of Rod Ends in the system
			systemState.numRodEnd++;
			partAccepted = true;
		}

		// Schedule a new arrival event
		listOfEvents.insert(it, Event('a', simulationTime + exponentialDist(interArr_RodEnd)));
		break;
	case 'b':
		if (prob < (accProb_Piston*100)) {
			// Increment the number of Pistons in the system
			systemState.numPiston++;
			partAccepted = true;
		}

		// Schedule a new arrival event
		listOfEvents.insert(it, Event('b', simulationTime + exponentialDist(interArr_Piston)));
		break;
	case 'c':
		if (prob < (accProb_CylinderCap*100)) {
			// Increment the number of Cylinder Caps in the system
			systemState.numCylinderCap++;
			partAccepted = true;
		}

		// Schedule a new arrival event
		listOfEvents.insert(it, Event('c', simulationTime + exponentialDist(interArr_CylinderCap)));
		break;
	case 'd':
		if (prob < (accProb_Cylinder*100)) {
			// Increment the number of Cylinders in the system
			systemState.numCylinder++;
			partAccepted = true;
		}

		// Schedule a new arrival event
		listOfEvents.insert(it, Event('d', simulationTime + exponentialDist(interArr_Cylinder)));
		break;
	case 'f':
		if (prob < (accProb_CylinderRodEnd*100)) {
			// Increment the number of Cylinder Rod Ends in the system
			systemState.numCylinderRodEnd++;
			partAccepted = true;
		}

		// Schedule a new arrival event
		listOfEvents.insert(it, Event('f', simulationTime + exponentialDist(interArr_CylinderRodEnd)));
		break;
	default:
		cout << "Error, bad input, quitting\n";
		break;
	}

	if (partAccepted == true) {

		// If there is at least 1 part of each, create a new assembly entity
		createAssemblyEntity();
	}
}

// Handle system changes when an assembly entity leaves the assembly station.
void departureAssembly(double ID) {

	// Reduce the number of assembly entities at the Assembly station or at the Assembly station queue by 1
	systemState.numAssembly_preAssembly--;

	// Increase the number of assembly entities in the Coating station or at the Coating station queue
	systemState.numAssembly_preCoat++;

	// Add this assembly entity to the Coating station queue
	coatingQueue.push(ID);

	// If the number of assembly entities at the Coating station or at the Coating station queue is 1, schedule a departure event
	// from coating station
	if (systemState.numAssembly_preCoat == 1) {

		// Departure event
		listOfEvents.insert(it, Event('y',
			simulationTime + normalDist(srvcTime_Coating_Mean, srvcTime_Coating_Stdev), coatingQueue.front()));

		// Pop out the part ID from the coating queue
		coatingQueue.pop();
	}
		
	// If the number of assembly entities at the Assembly station or at the Assembly station queue is at least 1, schedule a departure event
	// from Assembly station
	if (systemState.numAssembly_preAssembly >= 1) {

		// Departure event
		listOfEvents.insert(it, Event('x',
			simulationTime + normalDist(srvcTime_Assembly_Mean, srvcTime_Assembly_Stdev), assemblyStationQueue.front()));

		// Pop out the part ID from the assembly queue
		assemblyStationQueue.pop();
	}
	else {}

}

// Handle system changes when an assembly entity leaves the coating station
void departureCoating(double ID) {

	// Decrease the number of assembly entities in the Coating station or at the Coating station queue
	systemState.numAssembly_preCoat--;

	// If the number of assembly entities at the Coating station or at the Coating station queue is at least 1, schedule a departure event
	// from coating station
	if (systemState.numAssembly_preCoat >= 1) {

		// Departure event
		listOfEvents.insert(it, Event('y',
			simulationTime + normalDist(srvcTime_Coating_Mean, srvcTime_Coating_Stdev), coatingQueue.front()));

		// Pop out the part ID from the coating queue
		coatingQueue.pop();
	}

	////////////////////////////////////
	//       Inspection Station       //
	////////////////////////////////////
	
	// prob in the range 0 to 99. random number from 0 to 1 for use in determining whether assembly entity is accepted or rejected
	double prob = rand() % 100; 

	double acc_Prob; // the probability (from 0 to 1) of the Assembly entity being accepted
	bool isReWork = false; // true if the current part has undergone rework before, false otherwise

	// First determine the probability with an Assembly entity being accepted or rejected. This depends
	// on whether the Assembly entity has undergone rework or not.
	// If the list of Assemblies in the system that have undergone rework is 0, assing NoRework probability
	if (partIDsRework.empty() == true) {
		acc_Prob = accProb_Assembly_NoRework*100;
	}
	// If there are some assembly entities in the system that have undergone rework, see if the list contains this ID
	else if (std::find(partIDsRework.begin(), partIDsRework.end(), ID) != partIDsRework.end()) { 

		// If ID is found (affirmative), assign rework probability
		acc_Prob = accProb_Assembly_Rework*100;

		// Keep a note that this part has undergone rework before
		isReWork = true;
	}
	else {
		// If ID is NOT found, assign normal probability
		acc_Prob = accProb_Assembly_NoRework*100;
	}

	////////////////////////////////////////
	//				Routing				  //
	////////////////////////////////////////

	// Now determine whether the assembly entity is actually accepted, and determine routing based on the result
	if (prob < acc_Prob) { // If accepted
		
		// Increment the number of Assembly parts that have been delivered if simulation time is greater than ramp-up time
		if (simulationTime > rampUpTime)
			totalAssembliesDelivered++;

		// If the part was a re-work part, remove its ID from the list of parts in the system that have undergone rework
		if (isReWork == true)
			partIDsRework.remove(ID);


		// Add the total amount of time spent by this assembly entity in the system to the statistic of interest. Only if simulation time
		// is greater than ramp-up time
		if (simulationTime > rampUpTime) {
			it_creationTime = creationTimes.find(ID);
			if (it_creationTime != creationTimes.end()) {
				totalTimeAssembliesInSystem = totalTimeAssembliesInSystem + (simulationTime - it_creationTime->second);
			}
			else {
				cout << "Error with tagging creation times for Assemblies. Simulation time: " << simulationTime << endl;
			}
		}
		

		// Remove this ID from the map of IDs and creation times
		creationTimes.erase(creationTimes.find(ID));
	}

	// If the part is rejected, route it to the rework station.
	else {

		// Increment the number of parts in the ReWork station or in the ReWork station queue
		systemState.numAssembly_preReWork++;

		// Add this ID to the rework queue.
		reWorkQueue.push(ID);

		// If the number of parts in the ReWork station or in the ReWork station queue is 1, schedule a departure event
		if (systemState.numAssembly_preReWork == 1) {

			// Departure event
			listOfEvents.insert(it, Event('z',
				simulationTime + normalDist(srvcTime_Rework_Mean, srvcTime_Rework_Stdev), reWorkQueue.front()));

			// Pop out the part ID from the rework queue
			reWorkQueue.pop();
		}

	}
}

// Handle system changes when a part leaves the ReWork station
void departureReWork(double ID) {

	// Decrease the number of assembly entities in the ReWork station or at the ReWork station queue
	systemState.numAssembly_preReWork--;

	// Increase the number of assembly entities int he Assembly station or at the Assembly station queue
	systemState.numAssembly_preAssembly++;

	// If this part has not previously undergone rework, add it to the list of parts that have undergone rework
	if (std::find(partIDsRework.begin(), partIDsRework.end(), ID) == partIDsRework.end()) {
		partIDsRework.insert(it_rework, ID);
	}

	// Add this ID to the assembly station queue.
	assemblyStationQueue.push(ID);

	// If the number of assembly entities in the ReWork station or at the ReWork station queue is at least 1, schedule
	// a departure event
	if (systemState.numAssembly_preReWork >= 1) {

		// Departure event
		listOfEvents.insert(it, Event('z',
			simulationTime + normalDist(srvcTime_Rework_Mean, srvcTime_Rework_Stdev), reWorkQueue.front()));

		// Pop out the part ID from the rework queue
		reWorkQueue.pop();
	}

	// If the number of assembly entities in the Assembly station or the Assembly station queue is 1, schedule
	// a departure event
	if (systemState.numAssembly_preAssembly == 1) {

		// Departure event
		listOfEvents.insert(it, Event('x',
			simulationTime + normalDist(srvcTime_Assembly_Mean, srvcTime_Assembly_Stdev), assemblyStationQueue.front()));

		// Pop out the part ID from the assembly queue
		assemblyStationQueue.pop();
	}

}
