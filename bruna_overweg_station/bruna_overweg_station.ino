/* Geschreven door Pascal Scheffers, september 2017. 
 *
 * Synopsis:
 * Een circa 8 meter lange rechte treinbaan (DC) met wissel automaat:
 * De wissel automaat staat ingesteld om de trein van links naar rechts en omgekeerd
 * te laten rijden door elke X seconden de polariteit van de rails om te wisselen. 
 * Bij de eindstukken is telkens 1 rails onderbroken door een diode, zodat de trein daar
 * slechts een kant op kan rijden en dus betrouwbaar stopt.
 * 
 * De Arduino laat de overweg op het juiste moment knipperen en stopt de trein op het station.
 * 
 * De layout is alsvolgt:
 * 
 * LINKS----REEDSW_A---overweg--|-REEDSW_B--station--REEDSW_C-|------RECHTS
 * 
 * Lengtes:
 * Links naar rechts: 545 cm
 * Links naar A: 171 cm (inv. 216)
 *            | 31cm
 * Links naar B: 298 cm (inv. 247)
 *            | 127cm
 * Links naar C: 329 cm (inv. 374)
 *            | 216cm
 * Naar Rechts  545cm   
 *
 * 
 * Waar het stuk tussen de |---| onderbroken is en aangesloten is op de NC pins van een relais. 
 * Hierdoor zal als de Arduino niets detecteerd of vast loopt de trein altijd blijven rijden.
 * 
 * Technisch is het eenvoudig om inplaats van de richting sensor, de Arduino ook de richting te laten 
 * regelen met een wissel relais. 
 *  
 * Het programma is eenvoudig alsvolgt:
 * Als de trein van links naar rechts rijdt: (richting_links=true)
 * Bij REEDSW_A: overweg op rood
 *     REEDSW_B: overweg op wit
 *     REEDSW_C: stop trein voor STATION_WACHT_TIJD milliseconden
 *     
 * Als de trein van rechts naar links rijdt: (richting_links=false)
 * Bij REEDSW_C: doe niets
 *     REEDSW_B: 1. stop trein voor STATION_WACHT_TIJD milliseconden
 *               2. zodra de trein gaat rijden, overweg op rood
 *     REEDSW_A: Overweg wit.
 *     
 *     
 *     
 */
 
// Timings
#define OVERWEG_ROOD_INTERVAL 333 // milliseconden
#define OVERWEG_WIT_SNELHEID 4 // tijd tussen fader stapjes. Lager is sneller knipperen
#define OVERWEG_WIT_FREQUENTIE 1500 // tijd voor een hele knipper cyclus.

#define STATION_WACHT_TIJD 7000 // ms

/* Pin indeling */
/* Uitgangen */
#define OVERWEG_G_PIN 10  
#define OVERWEG_R_PIN 11
#define OVERWEG_K_PIN 9
#define OVERWEG_KK_PIN 12
#define STATION_STOP_RELAY 7

/* Ingangen */
// REED Switches, van links naar rechts:
#define REEDSW_A_PIN 2  // Direkt na de tunnel
#define REEDSW_B_PIN 4  // Tussen tunnel en station
#define REEDSW_C_PIN 5  // Tussen station en baaneinde

// Opto coupler baanrichting sensor
#define BAANRICHTING_PIN 3

// Interne API constanten
#define OVERWEG_KNIPPER_ONVERANDERD -1
#define OVERWEG_KNIPPER_WIT 0
#define OVERWEG_KNIPPER_ROOD 1
#define RICHTING_LINKS HIGH




void setup() {
  // Alle pinnen op de juiste modus instellen:
  
  pinMode(OVERWEG_G_PIN, OUTPUT);
  pinMode(OVERWEG_R_PIN, OUTPUT);
  pinMode(OVERWEG_K_PIN, OUTPUT);
  pinMode(OVERWEG_KK_PIN, OUTPUT);
  pinMode(STATION_STOP_RELAY, OUTPUT);
  pinMode(REEDSW_A_PIN, INPUT_PULLUP);
  pinMode(REEDSW_B_PIN, INPUT_PULLUP);
  pinMode(REEDSW_C_PIN, INPUT_PULLUP);
  pinMode(BAANRICHTING_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("Bruna Kerstspoor overweg & station controller gestart.");

}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long nu = millis();
  static unsigned long treinGestopt = 0;
  bool baanRichtingLinks = false;
  static bool vorigeBaanRichtingLinks = false;
  static bool treinHeeftGestopt = false;
  if (digitalRead(BAANRICHTING_PIN) == RICHTING_LINKS) {
    baanRichtingLinks = true;
  } 
  
  if (vorigeBaanRichtingLinks != baanRichtingLinks) {
    if (baanRichtingLinks) {
      Serial.println("Baanrichting nu links.");
    } else {
      Serial.println("Baanrichting nu rechts.");
    }
    if (treinGestopt > 0) {
      // Dit is een watchdog functie. Dit is de enige echt vervelende als het mis gaat, dus reset het relais
      // als de baanrichting veranderd. 
      digitalWrite(STATION_STOP_RELAY, LOW);
      treinGestopt = 0;
    }
    vorigeBaanRichtingLinks = baanRichtingLinks;
    treinHeeftGestopt = false;
    overwegBijwerken(nu, OVERWEG_KNIPPER_WIT); 
  }

  if (treinGestopt > 0 && nu - treinGestopt > STATION_WACHT_TIJD ) {
    digitalWrite(STATION_STOP_RELAY, LOW);
    treinGestopt = 0;
    Serial.println("Trein vertrekt van station.");
    if (!baanRichtingLinks) {
      overwegBijwerken(nu, OVERWEG_KNIPPER_ROOD); 
    }
  }
  
  overwegBijwerken(nu, OVERWEG_KNIPPER_ONVERANDERD);  
  if (digitalRead(BAANRICHTING_PIN) == RICHTING_LINKS) {
    // Trein gaat van links naar rechts.
    if (digitalRead(REEDSW_A_PIN) == LOW) {
      //Serial.print("A");
      overwegBijwerken(nu, OVERWEG_KNIPPER_ROOD); 
    }
    if (digitalRead(REEDSW_B_PIN) == LOW) {
      //Serial.print("B");
      overwegBijwerken(nu, OVERWEG_KNIPPER_WIT); 
    }
    if (!treinHeeftGestopt && treinGestopt == 0 && digitalRead(REEDSW_C_PIN) == LOW) {
      //Serial.print("C");
      digitalWrite(STATION_STOP_RELAY, HIGH);
      treinGestopt = nu;
      treinHeeftGestopt = true;
      Serial.println("Reedswitch C: trein gestopt");
      overwegBijwerken(nu, OVERWEG_KNIPPER_WIT); 
    }
  } else {
    if (!treinHeeftGestopt && treinGestopt==0 && digitalRead(REEDSW_B_PIN) == LOW) {
      //Serial.print("B");
      digitalWrite(STATION_STOP_RELAY, HIGH);
      treinGestopt = nu;
      treinHeeftGestopt=true;
      Serial.println("Reedswitch B: trein gestopt");
    }
    if (digitalRead(REEDSW_A_PIN) == LOW) {
      //Serial.print("A");
      overwegBijwerken(nu, OVERWEG_KNIPPER_WIT); 
    }
  }
  
}


void overwegBijwerken(unsigned long nu, char nieuweKnipperStand) {
  

  
  static unsigned long laatsteKnipper=0;
  static char witRichting = 1;
  static unsigned char witIntensiteit=20;
  static char knipperStand=OVERWEG_KNIPPER_WIT;
  static bool roodWissel=false;
  
  if (nieuweKnipperStand != OVERWEG_KNIPPER_ONVERANDERD && nieuweKnipperStand != knipperStand) {
    digitalWrite(OVERWEG_G_PIN, LOW);
    digitalWrite(OVERWEG_K_PIN, LOW);
    digitalWrite(OVERWEG_R_PIN, LOW);
    if (nieuweKnipperStand == OVERWEG_KNIPPER_WIT) {
      Serial.println("Overweg: wit");
      digitalWrite(OVERWEG_KK_PIN, LOW);
    } else {
      Serial.println("Overweg: rood");
      digitalWrite(OVERWEG_R_PIN, HIGH);
      digitalWrite(OVERWEG_KK_PIN, HIGH);
    }
      knipperStand = nieuweKnipperStand;
      laatsteKnipper = 0;
  }
  unsigned long knipperDuur = nu - laatsteKnipper;
  if (knipperStand == OVERWEG_KNIPPER_WIT) {
    float periodeStap = nu % OVERWEG_WIT_FREQUENTIE; // % is modulo.
    periodeStap = 2* M_PI * (periodeStap/OVERWEG_WIT_FREQUENTIE); 
    float nieuwWitF = abs((sin(periodeStap)+1)/2)*254;
    if (nieuwWitF > 240) nieuwWitF=255;
    //if (nieuwWitF < 3) nieuwWitF=3;
    
    int nieuwWit = (int)nieuwWitF;
    //Serial.println(sin(periodeStap), "%f");
    //Serial.println(nieuwWit, "%d");
    if (nieuwWit != witIntensiteit) {
      witIntensiteit = nieuwWit;
      analogWrite(OVERWEG_K_PIN,witIntensiteit);
    }
    /*
    if (witIntensiteit > 235) {
      witRichting = -1*2;
    } else if (witIntensiteit < 5) {
      witRichting = 1*2;
    }
    if (knipperDuur > OVERWEG_WIT_SNELHEID) {
      witIntensiteit += witRichting;
      analogWrite(OVERWEG_K_PIN,witIntensiteit);
      laatsteKnipper = nu;
    }*/
  } else {
    if (knipperDuur > OVERWEG_ROOD_INTERVAL) {
      if (roodWissel) {
        roodWissel =false;
        digitalWrite(OVERWEG_G_PIN, HIGH);
        digitalWrite(OVERWEG_R_PIN, LOW);
      } else {
        roodWissel = true;
        digitalWrite(OVERWEG_G_PIN, LOW);
        digitalWrite(OVERWEG_R_PIN, HIGH);
      }
      laatsteKnipper = nu;
    }
  } 

}

