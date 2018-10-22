// A quick and dirty timer
// Needs to come after blinkos.h to get millis_t


#define NEVER ( (millis_t)-1 )          // UINT32_MAX would be correct here, but generates a Symbol Not Found.

extern millis_t millis_snapshot;

class OS_Timer {

    private:

        millis_t m_expireTime;		// When this timer will expire

    public:

        OS_Timer() : m_expireTime(0) {};		// Timers come into this world pre-expired.

        bool isExpired();

        millis_t getRemaining();

        void set( millis_t ms );            // This time will expire ms milliseconds from now

        void add( uint16_t ms );

        void never(void);                   // Make this timer never expire (unless set())


};


inline bool OS_Timer::isExpired() {
    return millis_snapshot >= m_expireTime;
}

inline void OS_Timer::set( millis_t ms ) {
    m_expireTime= millis_snapshot+ms;
}

inline millis_t OS_Timer::getRemaining() {

    millis_t timeRemaining;

    if( millis_snapshot >= m_expireTime) {

        timeRemaining = 0;

    } else {

        timeRemaining = m_expireTime - millis_snapshot;

    }

    return timeRemaining;

}

inline void OS_Timer::add( uint16_t ms ) {

    // Check to avoid overflow

    millis_t timeLeft = NEVER - m_expireTime;

    if (ms > timeLeft ) {

        m_expireTime = NEVER;

        } else {

        m_expireTime+= ms;

    }
}

inline void OS_Timer::never(void) {
    m_expireTime=NEVER;
}


// Copy the internal millsecond counter to the global millis_snapshot
// Careful becuase the internal counter can be updated in the background

void updateMillisSnapshot(void);

// Note this runs in callback context. Call it once per ms.

void incrementMillis1ms(void);