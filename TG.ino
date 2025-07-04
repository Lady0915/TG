volatile int NumPulsos;
const int valvulaPin = 5;
const int PinSensor = 2;
const int pin_led = 9;
const int pin_pulsador = 11;
const int pin_pulsador2 = 12;

const float factor_conv = 7.11;
const float UMBRAL_CAUDAL = 0.001;

float volumen = 0;
float primerCaudalDetectado = 0;
bool volumenCalculado = false;

bool sistemaIniciado = false;
bool bombaEncendida = false;
bool caudalAnteriorMayor = false;

unsigned long tiempoInicioSistema = 0;
unsigned long tiempoInicioBomba = 0;
unsigned long duracionEncendido = 0;  // Duración variable: 500 ms o 10 s

const unsigned long duracionFaseInicial = 120000; // 120 segundos
const unsigned long duracionSensor = 1000;        // 1 segundos
const unsigned long duracionPulsador2 = 12000;   // 12 segundos

bool estadoAnteriorPulsador = HIGH;
bool estadoAnteriorPulsador2 = HIGH;

void ContarPulsos() {
  NumPulsos++;
}

int ObtenerFrecuencia() {
  NumPulsos = 0;
  interrupts();
  delay(1000);
  noInterrupts();
  return NumPulsos;
}

void setup() {
  Serial.begin(9600);
  pinMode(PinSensor, INPUT);
  attachInterrupt(digitalPinToInterrupt(PinSensor), ContarPulsos, RISING);
  pinMode(pin_led, OUTPUT);
  pinMode(valvulaPin, OUTPUT);
  digitalWrite(pin_led, LOW);
  digitalWrite(valvulaPin, HIGH);
  pinMode(pin_pulsador, INPUT_PULLUP);
  pinMode(pin_pulsador2, INPUT_PULLUP);
  estadoAnteriorPulsador = digitalRead(pin_pulsador);
  estadoAnteriorPulsador2 = digitalRead(pin_pulsador2);
  Serial.println("En espera para iniciar el llenado");
}

void loop() {
  bool estadoActualPulsador = digitalRead(pin_pulsador);
  bool estadoActualPulsador2 = digitalRead(pin_pulsador2);

  // --- INICIO del sistema con primer pulsador ---
  if (!sistemaIniciado && estadoAnteriorPulsador == HIGH && estadoActualPulsador == LOW) {
    sistemaIniciado = true;
    tiempoInicioSistema = millis();
    digitalWrite(valvulaPin, LOW);
    Serial.println("Electroválvula ABIERTA");
   digitalWrite(pin_led, HIGH);
    Serial.println("Sistema iniciado - Bomba encendida 120s");
  }
  estadoAnteriorPulsador = estadoActualPulsador;

  // --- FASE INICIAL  ---
  if (sistemaIniciado && millis() - tiempoInicioSistema < duracionFaseInicial) {
    return;
  }

  // Apagar la bomba si sigue encendida por la fase inicial
  if (sistemaIniciado && digitalRead(pin_led) == HIGH && !bombaEncendida) {
    digitalWrite(valvulaPin, HIGH);
    Serial.println("Electroválvula CERRADA");
    digitalWrite(pin_led, LOW);
    Serial.println("Fin fase inicial - sistema en espera permanente");
  }

  // --- CICLO PRINCIPAL POST-INICIO ---
  if (sistemaIniciado) {
    float frecuencia = ObtenerFrecuencia();
    float caudal_L_m = frecuencia / factor_conv;
    bool caudalActualMayor = (caudal_L_m > UMBRAL_CAUDAL);

    // --- Señal de caudal detectada (flanco ascendente) ---
    if (caudalActualMayor && !caudalAnteriorMayor && !bombaEncendida) {
      primerCaudalDetectado = caudal_L_m;
      bombaEncendida = true;
      volumenCalculado = false;
      tiempoInicioBomba = millis();
      duracionEncendido = duracionSensor;  // Medio segundo
      digitalWrite(valvulaPin, LOW);
      Serial.println("Electroválvula ABIERTA");
      digitalWrite(pin_led, HIGH);
      Serial.println("Caudal detectado - bomba activada 1s");
    }

    // --- Pulsador 2 PRESIONADO (flanco descendente) ---
    if (estadoAnteriorPulsador2 == HIGH && estadoActualPulsador2 == LOW && !bombaEncendida) {
      primerCaudalDetectado = 0;  // No hay caudal
      bombaEncendida = true;
      volumenCalculado = true;  // NO calcular volumen en este caso
      tiempoInicioBomba = millis();
      duracionEncendido = duracionPulsador2;  // 10 segundos
      digitalWrite(valvulaPin, LOW);
      Serial.println("Electroválvula ABIERTA");
      digitalWrite(pin_led, HIGH);
      Serial.println("Pulsador 2 presionado - bomba activada 10s");
    }

    // --- Mientras bomba está encendida ---
    if (bombaEncendida) {
      if (!volumenCalculado && primerCaudalDetectado > 0) {
        volumen += (primerCaudalDetectado / 60.0) * (duracionEncendido / 1000.0);
        volumenCalculado = true;
      }

      if (millis() - tiempoInicioBomba >= duracionEncendido) {
        bombaEncendida = false;
        digitalWrite(valvulaPin, HIGH);
        Serial.println("Electroválvula CERRADA");
        digitalWrite(pin_led, LOW);
        primerCaudalDetectado = 0;
        Serial.println("Bomba apagada");
      }
    }

    // Mostrar datos
    Serial.print("Caudal: ");
    Serial.print(caudal_L_m, 3);
    Serial.print(" Volumen: ");
    Serial.print(volumen, 3);
    Serial.println(" L");

    caudalAnteriorMayor = caudalActualMayor;
    estadoAnteriorPulsador2 = estadoActualPulsador2;
  }
}
