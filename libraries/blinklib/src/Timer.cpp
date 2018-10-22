
#include "ArduinoTypes.h"

#include "blinklib.h"


// Note we directly access millis() here, which is really bad style.
// The timer should capture millis() in a closure, but no good way to
// do that in C++ that is not verbose and inefficient, so here we are.

bool Timer::isExpired() {
    return millis() >= m_expireTime;
}

void Timer::set( uint32_t ms ) {
    m_expireTime= millis()+ms;
}

uint32_t Timer::getRemaining() {

    uint32_t timeRemaining;

    if( millis() >= m_expireTime) {

        timeRemaining = 0;

        } else {

        timeRemaining = m_expireTime - millis();

    }

    return timeRemaining;

}

void Timer::add( uint16_t ms ) {

    // Check to avoid overflow

    uint32_t timeLeft = NEVER - m_expireTime;

    if (ms > timeLeft ) {

        m_expireTime = NEVER;

        } else {

        m_expireTime+= ms;

    }
}

void Timer::never(void) {
    m_expireTime=NEVER;
}

