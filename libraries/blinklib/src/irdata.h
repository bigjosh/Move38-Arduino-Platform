


#define STATE_BROADCAST_SPACING_MS  50           // How often do we broadcast our state to neighboring tiles?

#define STATE_ABSENSE_TIMEOUT_MS 250             // If we don't get any state received on a face for this long, we set their state to 0

 // Called from timer on every click
 
 void updateIRComs(void);
 
 