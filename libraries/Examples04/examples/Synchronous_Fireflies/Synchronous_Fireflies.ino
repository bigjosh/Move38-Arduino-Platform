void setup() {
  // put your setup code here, to run once:

}

unsigned urge = 0;    // The urge to flash. Increases until we can't take it anymore!

void loop() {
  
        // put your main code here, to run repeatedly:
        
        static uint16_t power=0;         
        
        power++;
        
        if ( power >= 1000 ) {          

          irSendAllDibit( 1 );          // Flash to neighboors! (actually values does not matter)

          setRGBAll( 0 , 200 , 0 );     // Flash to onlookers!

          delay(100);

          urge = 0;

        }
        for(uint8_t face=0; face< FACE_COUNT; face++ ) {

          if ( irIsaAvailable( face ) ) {     // Anyone near us blink?

            irReadDibit( face );              // Ignore the Actual data

            ///***** THE MAGIC HAPPENS!!!!! *****////
              
            urge += (urge/10);                       
                
        }
    }
}
