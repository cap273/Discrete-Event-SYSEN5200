// Definition of an Event class.

#include <iostream>
#include <string>

// Include header file for class Event
#include "Event.h"

// Default contructor
Event::Event()
{
	eventType = 'w'; // This is not a real event.
	timeOfEvent = std::numeric_limits<double>::infinity();
	partID = 0; // This is an indicator that the partID number is not 'real'
}

// Constructor without a part ID associated with an assembly part
Event::Event(char type, double time)
{
	eventType = type;
	timeOfEvent = time;
	partID = 0; // This is an indicator that the partID number is not 'real'.

};

// Constructor without a part ID associated with an assembly part
Event::Event(char type, double time, double ID)
{
	eventType = type;
	timeOfEvent = time;
	partID = ID;
};

// Destructor
Event::~Event(void) {};

// Overloaded operator ==
bool Event::operator==(const Event &other) const
{
	return (eventType == other.eventType) && (timeOfEvent == other.timeOfEvent);
}

// Returns true if the event has a valid partID, false otherwise
bool  Event::hasValidPartID()
{
	if (partID == 0) {
		return false;
	}

	return true;
}

// Convert eventType to String
std::string Event::eventTypeToString()
{

	// 'a' for Rod End arrival
	// 'b' for Piston arrival
	// 'c' for Cylinder Cap arrival
	// 'd' for Cylinder arrival
	// 'f' for Cyinder Rod End arrival

	// 'e' for End of Simulation

	// 'x' for departure from Assembly station
	// 'y' for departure from Coating station
	// 'z' for departure from ReWork station

	if (eventType == 'a')
		return "Arrival: Rod End";
	if (eventType == 'b')
		return "Arrival: Piston";
	if (eventType == 'c')
		return "Arrival: Cylinder Cap";
	if (eventType == 'd')
		return "Arrival: Cylinder";
	if (eventType == 'f')
		return "Arrival: Cylinder Rod End";

	if (eventType == 'x')
		return "Departure: Assembly Station";
	if (eventType == 'y')
		return "Departure: Coating Station";
	if (eventType == 'z')
		return "Departure: ReWork Station";

	if (eventType == 'e')
		return "End of simulation";

	return "Error";
}

// Getter functions
char Event::getEventType()
{
	return eventType;
};

double Event::getTimeOfEvent()
{
	return timeOfEvent;
};

double Event::getPartID()
{
	return partID;
}

// Setter functions
void Event::setTimeOfEvent(double time)
{
	timeOfEvent = time;
}

void Event::setEventType(char type)
{
	eventType = type;
}

void Event::setPartID(double ID)
{
	partID = ID;
}