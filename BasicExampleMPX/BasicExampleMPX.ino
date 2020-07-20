#define analogPin 15
#define AMOSTRAS 50

int Pressao;
int numbersI[AMOSTRAS];
int analogR;

void setup() {
  pinMode(analogPin, INPUT);

  Serial.begin(115200);

}

void loop() {
  analogR = analogRead(analogPin)*100;

  Pressao = filtroMediaMovel();

  Serial.print(analogR);
  Serial.print(" ");
  Serial.println(Pressao);

  delay(20);

}

long filtroMediaMovel(){
  for(int i = AMOSTRAS - 1 ; i > 0; i--) numbersI[i] = numbersI[i-1];

  numbersI[0] = analogR;

  long acc = 0;

  for(int i = 0; i < AMOSTRAS; i++) acc += numbersI[i];

  return acc/AMOSTRAS;
}
