// Header file for class Event
#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <string>
#include <math.h>       /* isfinite, sqrt */

class Event {
public:

	// Default constructor
	Event();

	// Constructor declaration without a ID number associated with an Assembly part
	Event(char, double);

	// Constructor declaration including an ID number associated with an Assembly part
	Event(char, double, double);

	// Destructor declaration
	~Event();

	// Overloaded operator ==
	bool operator==(const Event &other) const;

	// Convert eventType to String
	std::string eventTypeToString();

	// Returns true if the event has a valid partID, false otherwise
	bool hasValidPartID();

	// Member function declarations (getters and setters)
	double getTimeOfEvent();
	char getEventType();
	double getPartID();

	void setTimeOfEvent(double);
	void setEventType(char);
	void setPartID(double);

private:
	// Variable declarations.

	// in minutes
	double timeOfEvent;
	
	/////////////
	// eventType is a char that describes the type of event
	////////////

	// 'a' for Rod End arrival
	// 'b' for Piston arrival
	// 'c' for Cylinder Cap arrival
	// 'd' for Cylinder arrival
	// 'f' for Cyinder Rod End arrival

	// 'e' for End of Simulation

	// 'x' for departure from Assembly station
	// 'y' for departure from Coating station
	// 'z' for departure from ReWork station
	char eventType;

	// Part ID number associated with an Assembly
	double partID;
};

#endif /* EVENT_H */