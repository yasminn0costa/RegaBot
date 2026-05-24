#include <DHT.h>


#define PIN_SOLO      A0      // YL-69 (SOLO)
#define PIN_BOIA      7       // BOIA  (POSSIVELMENTE VAI SER ALTERADO QUANDO SOUBER O MODELO)
#define PIN_DHT       4       // DHT11 (temperatura e umidade do ar)

#define BAUD_SERIAL   9600
#define BAUD_SERIAL1  9600


DHT dht(PIN_DHT, DHTTYPE);


int   umidadeSoloPercentual = 0;
int   nivelAguaPercentual   = 0;
float temperatura = 0.0;
float umidadeAr   = 0.0;


void lerSensores();
void enviarDadosApp();


void setup() {
  Serial.begin(BAUD_SERIAL);
  Serial1.begin(BAUD_SERIAL1);

  dht.begin();
  pinMode(PIN_BOIA, INPUT_PULLUP);  

  Serial.println("RegaBot - Módulo de Sensores Iniciado");
}


// coletar dados pelos sensores e enviar para o app
void loop() {
  bool isLeiturasValidas = lerSensores();      

  if (!isLeiturasValidas) {
    Serial.println("Erro na leitura dos sensores");
  }

  printSensores();
  enviarDadosApp();   
  delay(2000);        
}


// faz o mapeamento e a restricao do valor do sensor
// entre 0-876 para 0-100. retorna -1 se houver valores,
// no final, fora de 0-100. 
int normalisarUmidadeSolo(int valBrutoSolo) {
  int umidadeSolo = 0;

  umidadeSolo = map(valBrutoSolo, 0, 876, 0, 100);
  umidadeSolo = constrain(umidadeSolo, 0, 100);

  // checa se o valor esta dentro do range
  if ((umidadeSolo < 0) || (umidadeSolo > 100)) {
    return -1;
  }

  return umidadeSolo;
}


void lerBoia() {
  int estadoBoia = digitalRead(PIN_BOIA);

  if (estadoBoia == HIGH) {
    nivelAguaPercentual = 100;   
  } else {
    nivelAguaPercentual = 0;     
  }
}


// lê e normaliza a umidade do solo
bool lerUmidadeSolo() {
  int valBrutoSolo = analogRead(PIN_SOLO);

  umidadeSoloPercentual = normalisarUmidadeSolo(valBrutoSolo);

  if (umidadeSoloPercentual == -1) {
    Serial.println("Valor da umidade do solo não pode ser normalisada");
    return false;
  } else {
    return true;
  }
}


//  caso seja necessario debug no monitor serial (computador) 
void printSensores() {
  Serial.println("--- Leitura dos Sensores ---");

  Serial.print("Solo (bruto): ");
  Serial.print(valBrutoSolo);

  Serial.print(" -> Solo (%): ");
  Serial.println(umidadeSoloPercentual);

  Serial.print("Nível água: ");
  Serial.println(nivelAguaPercentual);

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");

  Serial.print("Umidade ar: ");
  Serial.print(umidadeAr);
  Serial.println(" %");

  Serial.println("----------------------------");
}


// lê os valores do DHT e checa se são válidos,
// se nao forem, retorna false, senao true.
bool lerDHT(DHT dht) {
  float brutoTemp     = dht.readTemperature();
  float brutoHumidade = dht.readHumidity();

  if ( !isnan(t) && !isnan(h) ) {
    temperatura = brutoTemp;
    umidadeAr   = brutoHumidade;
    return true;
  } else {
    Serial.println("Erro na leitura do DHT11");
    return false;
  }
}


// faz a leitura de todos os sensores.
// Sucesso: retorna true.
// Erro: retorna false.
bool lerSensores() {
  lerBoia();

  bool isSoloValido = lerUmidadeSolo();
  if (!isSoloValido) return false;

  bool isDHTValida = lerDHT();
  if (!isDHTValida) return false;
}


//  caso seja necessario debug no monitor serial (computador) 
void printDebugEnvioDados() {
  Serial.print("Enviado ao app: ");
  Serial.print(temperatura); Serial.print("|");
  Serial.print(umidadeAr); Serial.print("|");
  Serial.print(umidadeSoloPercentual); Serial.print("|");
  Serial.println(nivelAguaPercentual);
}


void enviarDadosApp() {
  Serial1.print(temperatura);
  Serial1.print("|");
  Serial1.print(umidadeAr);
  Serial1.print("|");
  Serial1.print(umidadeSoloPercentual);
  Serial1.print("|");
  Serial1.println(nivelAguaPercentual);

  printDebugEnvioDados();
}
