//Mascara de BITS

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000

#include <stdio.h>  
#include <stdlib.h>
#include <stdint.h>  // para uso de uint8_t
#include <LiquidCrystal.h> //Biblioteca para uso do DisplayLCD

//Parametros para Comunicação Serial

#define FOSC 16000000U
#define BAUD 9600
#define MYUBRR FOSC / 16 / (BAUD - 1)
#define TAMANHO 50

//Inicialização das variaveis para UART.

char msg_tx[20];
char msg_rx[32];
int pos_msg_rx = 0;
int tamanho_msg_rx = TAMANHO;

//Construtores das Funções UART

void UART_config(unsigned int ubrr);
void UART_Transmit(char *dados); 

int leituraAD(uint8_t pinoAD) { //Leitura e Conversão AD
  int valorLido;

  ADMUX = pinoAD;          // Configura Pino Conversor Analogico
  ADMUX |= (1 << REFS0);   // usa AVcc como referencia
  ADMUX &= ~(1 << ADLAR);  // resolução de 10 bit

  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // prescaler de 128
  ADCSRA |= (1 << ADEN);                                 // Habilita ADC

  ADCSRA |= (1 << ADSC);  // Inicia conversão

  while (ADCSRA & (1 << ADSC));  

  valorLido = ADCL;
  valorLido = (ADCH << 8) + valorLido;

  return valorLido;
}

int menuLCD(int x) { //Inicializador do LCD e Menu

  LiquidCrystal lcd(12, 11, 7, 6, 5, 4); //Configura pinos do LCD

  lcd.begin(16, 2); //Inicializa Display

  int porcentLeitura = (100 * static_cast<u32>(leituraAD(0)) / 1023);

  switch (x) {  //Menu LCD
    
    case 1:

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Projeto E209");
      lcd.setCursor(0, 1);
      lcd.print("Augusto/Matheus");

      break;

    case 2:

      lcd.setCursor(0, 0);
      lcd.print("Humidade");
      lcd.setCursor(0, 1);
      lcd.print("do solo:");
      lcd.print(porcentLeitura);
      lcd.print("% ");

      if (porcentLeitura < 50) {

        lcd.print("SECO");

      } else if (porcentLeitura < 70) {

        lcd.print("OK");

      } else {

        lcd.print("BOM");
      }

      break;
  }
}

//int leitura_MIN = 0; // Valor minimo fornecido pelo sensor
int leitura_MAX = 876; // Valor maximo fornecido pelo sensor
int leituraSensor = 0; // Variavel que armazena leitura AD
int valor_ON = 219; //Valor que liga bomba de água.

int main(void) { // Setup

  cli();

  //Inicialização da Serial
  UART_config(MYUBRR);

  DDRB |= (BIT0);   //PB0 definido como saida
  PORTD |= (BIT2 + BIT3);	//PD2 E PD3 ativado PULL UP
  PORTB |= (BIT5);  //PB5 ativado PULL UP
  PORTC |= (BIT1 + BIT2); //PC1 E PC2 ativado PULL UP
  PORTB &= ~(BIT0);  //desativa PB0

  EICRA = BIT0 + BIT1 + BIT3 + BIT4; //Habilita Transição de descida em INT0 E INT1

  EIMSK = BIT0 + BIT1; //Habilita INT0 e INT1

  sei(); //Habilita interrupção global

  while (1) { //Loop Infinito

    leituraSensor = leituraAD(0); // Lê sinal porta analogica PC0
    int porcentLeitura = (100 * static_cast<u32>(leituraSensor)) / 1023; //Converte sinal lido em porcentagem.

    int seletor = !(PINB & BIT5); //Seleciona modo de operação entre manual e automatico

    if (seletor == 0) {  //Se seletor no modo manual (desativado)

      if (valor_ON >= leituraSensor) {

        PORTB |= (BIT0);
      } else if (leitura_MAX <= leituraSensor) {

        PORTB &= ~(BIT0);
      }

    }

    else {  //Senão Modo manual

      int liga = !(PINC & BIT1);    //Lê botão liga
      int desliga = (PINC & BIT2);  //Lê botão desliga

      if (liga == 1) {  //se botão LIGA ativado

        PORTB = PORTB | BIT0;  // liga PB0

      }

      else if (desliga == 0) {  //se botão desliga ativado

        PORTB = PORTB & ~(BIT0);  // desliga PB0

      }
    }


    //Transmissão Serial de Texto
    UART_Transmit("Humidade do Solo: ");

    //Transmissão Serial de Números
    int X = porcentLeitura;
    itoa(X, msg_tx, 10);
    UART_Transmit(msg_tx);

    UART_Transmit("%\n");
  }
}

ISR(INT0_vect) { // Interrupção INT0

    menuLCD(1);
  
}

ISR(INT1_vect) { // Interrupção INT1

    menuLCD(2);

  }
   
//Transmissão de Dados Serial
void UART_Transmit(char *dados) {

  // Envia todos os caracteres do buffer dados ate chegar um final de linha
  while (*dados != 0) {
    while ((UCSR0A & BIT5) == 0)
      ;  // Aguarda a transmissão acabar

    // Escreve o caractere no registro de tranmissão
    UDR0 = *dados;
    // Passa para o próximo caractere do buffer dados
    dados++;
  }
}

//Configuração da Serial
void UART_config(unsigned int ubrr) {

  // Configura a  baud rate
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;
  // Habilita a recepcao, tranmissao e interrupcao na recepcao */
  UCSR0B = (BIT7 + BIT4 + BIT3);
  // Configura o formato da mensagem: 8 bits de dados e 1 bits de stop */
  UCSR0C = (BIT2 + BIT1);
}
