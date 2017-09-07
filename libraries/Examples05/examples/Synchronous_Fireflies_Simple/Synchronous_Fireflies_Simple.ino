void setup() {
  // put your setup code here, to run once:
}

unsigned urge = 0;    // The urge to flash. Increases until we can't take it anymore!

void loop() {
  
        // put your main code here, to run repeatedly:
        
        urge++;

        if (urge > 19000 ) {
                    
          setAllRGB( 0 , 200 , 0 );             

        } else {

          setAllRGB( 0 , 0 , 0 );             
          
        }
        
        if ( urge >= 20000 ) {          

          irSendAllDibit( 1 );          // Flash to neighboors! (actually values does not matter)

          urge = 0;

        }

        for(uint8_t face=0; face< FACE_COUNT; face++ ) {

          if ( irIsAvailable( face ) ) {     // Anyone near us blink?

            irReadDibit( face );              // Ignore the Actual data

            ///***** THE MAGIC HAPPENS!!!!! *****////
              
            urge += (urge/10);             // peer preasure boost
                
        }
    }


    
}
