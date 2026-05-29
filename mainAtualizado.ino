#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>

#define PIN_SOLO   A0      //YL-69 (SOLO)
#define PIN_BOIA   7       // BOIA (POSSIVELMENTE VAI SER ALTERADO QUANDO SOUBER O MODELO)
#define PIN_DHT    4       // DHT11 (temperatura e umidade do ar)
#define DHTTYPE    DHT11
#define rele 5         //pino do modulo rele

DHT dht(PIN_DHT, DHTTYPE);  //objeto da classe DHT
RTC_DS3231 rtc;             //objeto da classe RTC_DS3231

float temperatura = 0.0;          //variável global para a temperatura do ar
float umidadeAr = 0.0;            //variável global para a umidade do ar
int umidadeSoloPercentual = 0;    //variável global para a umidade do solo em porcentagem
int nivelAguaPercentual = 0;      //variável global para indicar se o nível de água está baixo
int dados[5];                     //vetor para armazenar as configurações enviadas pelo app. Disposicao dos dados: {modo_de_operação, horas, minutos, acionamento_manual_da_bomba}
bool flagBomba = false;           //flag para garantir que a bomba só seja acionada uma vez na IrrigaçãoPorHorario                   

void lerSensores();               //função que vai ler os dados dos sensores
void enviarDadosApp();            //função que envia os dados para o aplicativo
void LigarBomba();                //função que liga a bomba sempre que chamada (Yasminn)
void IrrigacaoPorHorario();       //função que liga a bomba segundo um horário definido no aplicativo
void LerDadosApp();               //função para ler as configurações enviados pelo aplicativo

void setup() {
  Serial.begin(9600);                      //inicia a comunicação serial com o monitor
  Serial1.begin(9600);                     //inicia a comunicação serial com o módulo bluetooth (HC-05)
  dht.begin();                             //inicia o objeto referente ao DHT11
  rtc.begin();                             //inicia o objeto referente ao módulo RTC DS3231
  pinMode(PIN_BOIA, INPUT_PULLUP);         //configura o pino conectado à boia
  pinMode(rele, OUTPUT);
  Serial.println("RegaBot - Módulo de Sensores Iniciado");
  rtc.adjust(DateTime(2026, 5, 26, 11, 31, 00));              //ajustar tempo: ano, mes, dia, horas, minutos, segundos. Ajustar apenas uma vez
  delay(100);
}

void loop() {
  lerSensores();
  LerDadosApp();
  switch(dados[0]){   //o primeiro elemento de dados[] armazena o modo de operação; 1 para irrigação por horário, 0 para irrigação automática
    case 1: IrrigacaoPorHorario(); break;
    case 0: //(Ea-Nazir) adicione aqui a chamada da função que vai fazer a irrigacao de forma automatica
    default: break;
  }
  enviarDadosApp();   
  delay(2000);        
}
//-----função de leitura dos sensores------
void lerSensores() {
  
  int valorBrutoSolo = analogRead(PIN_SOLO);
  umidadeSoloPercentual = map(valorBrutoSolo, 0, 876, 0, 100);
  umidadeSoloPercentual = constrain(umidadeSoloPercentual, 0, 100);
 
  int estadoBoia = digitalRead(PIN_BOIA);
  if (estadoBoia == HIGH) {
    nivelAguaPercentual = 100;   
  } else {
    nivelAguaPercentual = 0;     
  }

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temperatura = t;
    umidadeAr = h;
  } else {
    Serial.println("Erro na leitura do DHT11");
  }

  //  caso seja necessario debug no monitor serial (computador) 
  Serial.println("--- Leitura dos Sensores ---");
  Serial.print("Solo (bruto): "); Serial.print(valorBrutoSolo);
  Serial.print(" -> Solo (%): "); Serial.println(umidadeSoloPercentual);
  Serial.print("Nível água: "); Serial.println(nivelAguaPercentual);
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Umidade ar: "); Serial.print(umidadeAr); Serial.println(" %");
  Serial.println("----------------------------");
}

//------funcao de envio de dados --------------
void enviarDadosApp() {                             //envia os dados dos sensores e do horário definido usando o caracter "|" como delimitador
  Serial1.print(temperatura);               
  Serial1.print("|");
  Serial1.print(umidadeAr);
  Serial1.print("|");
  Serial1.print(umidadeSoloPercentual);
  Serial1.print("|");
  Serial1.print(nivelAguaPercentual);
  Serial1.print("|");
  Serial1.print(dados[1]);
  Serial1.print("|");
  Serial1.print(dados[2]);

  //  caso seja necessario debug no monitor serial (computador) 
  Serial.print("Enviado ao app: ");
  Serial.print(temperatura); Serial.print("|");
  Serial.print(umidadeAr); Serial.print("|");
  Serial.print(umidadeSoloPercentual); Serial.print("|");
  Serial.print(dados[1]); Serial.print("|");
  Serial.print(dados[2]); Serial.print("|");
  Serial.println(nivelAguaPercentual);
}

void LerDadosApp(){
  if(Serial1.available() > 0){                                          //verifica se há dados disponíveis no buffer
    int delimitador;                                                    //variável para pegar o indicice do caracter "|"
    String DadosNaoTratados = Serial1.readString();                     //variável que recebe a string com os dados
    for(int i=0; i < 4; i++){
      delimitador = DadosNaoTratados.indexOf('|');                      //recebe o indice do primeiro "|" na string
      if(delimitador >= 0){
        String valor = DadosNaoTratados.substring(0,delimitador);       //recebe o primeiro pedaço da string contendo o primeiro dado
        dados[i] = valor.toInt();                                       //converte o valor para inteiro
        DadosNaoTratados = DadosNaoTratados.substring(delimitador + 1); //cria uma nova string com os dados restantes
      }else{
        dados[i] = DadosNaoTratados.toInt();                            //quando não há mais "|" recebe o último dado
      }
    }
  }
}

void IrrigacaoPorHorario(){
  unsigned long inicio;
  DateTime agora = rtc.now();                                               //lê o horário atual
  if((dados[1]==agora.hour())&&(dados[2]==agora.minute())&&!flagBomba){     //compara se o horário atual é igual ao horário definido para ligar a bomba
    LigarBomba();                                                           //chama a função LigarBomba()
    flagBomba = true;                                                       //muda o valor do flag para true, assim a bomba não liga novamente mesmo dentro do horário
    inicio = millis();
  }
  if(flagBomba && (millis()-inicio)>=60000){                                //a bomba só pode ser ativada novamente após 1 minuto 
    flagBomba = false;
  }
}

void LigarBomba(){ //ainda sofrerá alterações

  digitalWrite(rele, HIGH); //liga a bomba toda vez que a função for chamada
  delay(6000);
  digitalWrite(rele, LOW);  //depois de um intervalo de tempo a bomba será desligada
}
