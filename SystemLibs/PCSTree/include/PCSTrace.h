#ifndef PCS_TRACE_H
#define PCS_TRACE_H

#define PCSTraceBuffSize 256

// NOTE: you need to set your project settings
//       Character Set -> Use Multi-Byte Character Set

// Singleton helper class
class PCSTrace
{
public:
	// displays a printf to the output window
	static void out(char* fmt, ...);

	// Big four
	PCSTrace() = default;
	PCSTrace(const PCSTrace &) = default;
	PCSTrace & operator = (const PCSTrace &) = default;
	~PCSTrace() = default;

private:
	static PCSTrace *privGetInstance();
	char privBuff[PCSTraceBuffSize];
};


#endif