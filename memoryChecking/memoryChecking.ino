// Deklarasi pin untuk relay dan tombol toggle
const int relayPin = 15;
const int toggleButtonPin = 2;

// Variabel untuk menyimpan status relay
bool relayState = LOW;

void setup() {
  // Set pin relay sebagai OUTPUT
  pinMode(relayPin, OUTPUT);
  // Set pin toggleButtonPin sebagai INPUT_PULLUP agar nilai awal HIGH
  pinMode(toggleButtonPin, INPUT_PULLUP);

  // Inisialisasi status relay sesuai nilai awal
  digitalWrite(relayPin, relayState);
}

void loop() {
  // Baca status tombol toggle
  int buttonState = digitalRead(toggleButtonPin);

  // Jika tombol ditekan (nilai LOW karena INPUT_PULLUP)
  if (buttonState == LOW) {
    // Ubah status relay (toggle)
    Serial.println("Tombol ditekan");
    relayState = !relayState;
    // Update output relay sesuai status baru
    digitalWrite(relayPin, relayState);
    // Tunda untuk menghindari bouncing pada tombol
    delay(250);
  }
}
