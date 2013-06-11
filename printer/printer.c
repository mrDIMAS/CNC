/*
 * printer.c
 *
 * Created: 08.06.2013 19:38:39
 *  Author: mrDIMAS
 */ 


#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#define bit_true( num, bit )  ( num |=  ( 1 << bit ))
#define bit_false( num, bit ) ( num &= ~( 1 << bit ))

#define STEPPERS_COUNT 3

#define SF_USE_HIGH_4_BITS 0x01
#define SF_CW_DIR		   0x02

#define S_MAX_LOW_BIT  0b00001000
#define S_MAX_HIGH_BIT 0b10000000

#define STEPPER_TIMER_1024_PRESCALER (( 1 << CS00 ) | ( 1 << CS02 ))
#define STEPPER_TIMER_256_PRESCALER  (( 1 << CS02 ))
#define STEPPER_TIMER_64_PRESCALER   (( 1 << CS00 ) | ( 1 << CS01 ))
#define STEPPER_TIMER_8_PRESCALER    (( 1 << CS01 ))
#define STEPPER_TIMER_NO_PRESCALER    ( 0 )

#define F_CPU 8000000UL

#define BAUDRATE ( 9600 )
#define BAUD_PRESCALE ( ( F_CPU / 16 / BAUDRATE ) - 1)

#define RECEIVE_BUFFER_SIZE ( 255 )
#define TRANSMIT_BUFFER_SIZE ( 128 )

volatile const uint8_t g_dir_bits[ 2 ][ 4 ] = { { 0x01, 0x02, 0x04, 0x08 }, 
												{ 0x08, 0x04, 0x02, 0x01 } };

volatile int8_t receiveBuffer[ RECEIVE_BUFFER_SIZE ]     = { 0 };
volatile int8_t transmitBuffer[ TRANSMIT_BUFFER_SIZE ]   = { 0 };
int8_t commandBuffer[ RECEIVE_BUFFER_SIZE ]   = { 0 };
	
volatile uint8_t receiveTail = 0;
volatile uint8_t receiveHead = 0;
volatile uint8_t receiveSize = 0;

volatile uint8_t transmitTail = 0;
volatile uint8_t transmitHead = 0;
volatile uint8_t transmitSize = 0;

#define X_STEPPER ( 0 )
#define Y_STEPPER ( 1 )
#define Z_STEPPER ( 2 )

volatile uint8_t busy = 0;
volatile uint8_t commandDone = 1;
volatile uint8_t drawLineMode = 0;

int8_t stepper_x_mult = 2;

// bresenham algorithm variables
int16_t x0	= 0;
int16_t y0	= 0;
int16_t x1	= 0;
int16_t y1	= 0;
int16_t dx	= 0;
int16_t dy	= 0;
int8_t  sx	= 0;
int8_t  sy	= 0;
int16_t err = 0;
int16_t e2  = 0;

int16_t g_x = 0;
int16_t g_y = 0;

typedef struct
{
	uint8_t flags;
	uint8_t counter;
	
	uint16_t stepsDone;
	uint16_t stepsToDo;
	
	volatile uint8_t * out_port;
} stepper_t;

volatile stepper_t * g_steppers[ STEPPERS_COUNT ] = { 0, 0, 0 };

void serial_init()
{
	UCSRA = 0;
	
	UCSRB |= ( 1 << RXEN )  | ( 1 << TXEN ) | ( 1 << RXCIE );

	UCSRC |= ( 1 << URSEL ) | ( 1 << UCSZ0 ) | ( 1 << UCSZ1 );
	
	UBRRH = ( BAUD_PRESCALE >> 8);
	UBRRL =   BAUD_PRESCALE ;
	
	receiveTail = 0;
	receiveHead = 0;
	receiveSize = 0;

	transmitTail = 0;
	transmitHead = 0;
	transmitSize = 0;
}

// Получение принятых данных из буфера
int8_t serial_get( )
{
	ATOMIC_BLOCK( ATOMIC_FORCEON )
	{
		int8_t data = 0;
		
		if( receiveSize > 0 )
		{
			data = receiveBuffer[ receiveHead ];
			
			receiveHead++;
			
			if( receiveHead >= RECEIVE_BUFFER_SIZE )
				receiveHead = 0;
			
			receiveSize--;
		}
		
		return data;
	}
}

// Отправка данных
void serial_send( int8_t data )
{
	ATOMIC_BLOCK( ATOMIC_FORCEON )
	{
		if( transmitSize < TRANSMIT_BUFFER_SIZE )
		{
			transmitBuffer[ transmitTail ] = data;
			
			transmitTail++;
			
			if( transmitTail == TRANSMIT_BUFFER_SIZE )
				transmitTail = 0;
			
			transmitSize++;
			
			UCSRB |= ( 1 << UDRIE );
		}
	}
}

// Получение данных и запись их в буфер
ISR( USART__RXC_vect )
{
	if( receiveSize < RECEIVE_BUFFER_SIZE )
	{
		receiveBuffer[ receiveTail ] = UDR;

		//serial_send( receiveBuffer[ receiveTail ] );//echo
		
		receiveTail++;
		
		if( receiveTail == RECEIVE_BUFFER_SIZE )
		receiveTail = 0;
		
		receiveSize++;
		
		
	}
}

// Передача данных из буфера по прерыванию
ISR( USART__UDRE_vect )
{
	if( transmitSize > 0 )
	{
		if( transmitSize > 0 )
		{
			UDR = transmitBuffer[ transmitHead ];
			
			transmitHead++;
			
			if( transmitHead == TRANSMIT_BUFFER_SIZE )
			transmitHead = 0;
			
			transmitSize--;
			
			if( transmitSize <= 0 )
				UCSRB &= ~( 1 << UDRIE );
		}
	}
}
	
void stepper_init( stepper_t * stepper, volatile uint8_t * out_port, uint8_t flags )
{
	stepper->flags = flags;
	stepper->out_port = out_port;
	stepper->stepsDone = 0;
	stepper->stepsToDo = 0;
	stepper->counter = 0;
	
	if( stepper->flags & SF_USE_HIGH_4_BITS )
		(*stepper->out_port) = g_dir_bits[ 0 ][ 0 ] << 4;
	else 
		(*stepper->out_port) = g_dir_bits[ 0 ][ 0 ];
}

void stepper_do_steps( stepper_t * stepper, uint16_t stepsToDo )
{
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		stepper->stepsDone = 0;
		stepper->stepsToDo = stepsToDo;
	}
}

void stepper_add_steps_to_do( stepper_t * stepper, uint16_t stepsToDo )
{
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		stepper->stepsToDo += stepsToDo;
	}
}

void stepper_set_direction( stepper_t * stepper, int8_t dir )
{
	if( dir < 0 )
		stepper->flags &= ~SF_CW_DIR;
	else
		stepper->flags |= SF_CW_DIR;
}

void stepper_stop( stepper_t * stepper )
{
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		if( stepper->flags & SF_USE_HIGH_4_BITS )
			(*stepper->out_port) &= 0x0F;
		else
			(*stepper->out_port) &= 0xF0;
		
		stepper->stepsDone = 0;
		stepper->stepsToDo = 0;
	}
}

uint8_t stepper_is_ready( stepper_t * stepper )
{
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		return stepper->stepsDone >= stepper->stepsToDo;
	}			
}

void stepper_update( stepper_t * stepper )
{	
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		if( stepper_is_ready( stepper ) )
		{
			stepper_stop( stepper );
			
			return;
		}
	}
		
	uint8_t direction = stepper->flags & SF_CW_DIR ? 0 : 1;
	
	if( stepper->flags & SF_USE_HIGH_4_BITS )
	{
		(*stepper->out_port) &= 0x0F;
		(*stepper->out_port) |= ( g_dir_bits[ direction ][ stepper->counter ] << 4 ) ;
	}		
	else
	{
		(*stepper->out_port) &= 0xF0;
		(*stepper->out_port) |= ( g_dir_bits[ direction ][ stepper->counter ] ) ;	
	}
			
	stepper->counter++;
	
	if( stepper->counter > 3 )
	{
		stepper->counter = 0;
		
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
		{	
			stepper->stepsDone++;
		}		
	}		
}	

void init_stepper_control_timer() 
{
	TIMSK |= ( 1 << TOIE0 );
	TCCR0 |= STEPPER_TIMER_8_PRESCALER;
}

volatile stepper_t g_x_stepper;
volatile stepper_t g_y_stepper;
volatile stepper_t g_z_stepper;

volatile uint8_t timer = 0;

void line( int16_t _x0, int16_t _y0, int16_t _x1, int16_t _y1 )
{
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		x0 = _x0 * stepper_x_mult;
		y0 = _y0;
		x1 = _x1 * stepper_x_mult;
		y1 = _y1;
		
		dx = abs( x1 - x0 );
		dy = abs( y1 - y0 );
		sx = x0 < x1 ? 1 : -1;
		sy = y0 < y1 ? 1 : -1;
		
		err = dx - dy;
		
		busy = 0xFF;
		
		stepper_set_direction( g_steppers[ X_STEPPER ], sx );
		stepper_set_direction( g_steppers[ Y_STEPPER ], sy );		
	}
}

char * receive_buffer_get_string( )
{	
	
	// clear command buffer
	for( uint8_t i = 0; i < RECEIVE_BUFFER_SIZE; i++ )
	{
		commandBuffer[ i ] = 0;
	}
		
		
	// copy command from serial to the command buffer
	uint8_t n = 0;	
	
	while( 1 ) // until command full received
	{
		commandBuffer[ n ] = serial_get( );
		
		if( commandBuffer[ n ] != 0 )
		{
			if( commandBuffer[ n ] == 'e' )
			{
				return &commandBuffer[ 0 ];
			}
						
			serial_send( commandBuffer[ n ] );
			
			n++;
		}			
	}
	
	return 0;	
	/*
	uint8_t end_of_string = 0;
	 
	// check - is the buffer contain string? String must contain 'e' at the end
	if( receiveHead < receiveTail )
	{
		for( uint8_t i = receiveHead; i < RECEIVE_BUFFER_SIZE; i++ )
		{
			if( receiveBuffer[ i ] == 'e' )
			{
				end_of_string = i;
			}
		}
	}
	else
	{
		for( uint8_t i = receiveHead; i < RECEIVE_BUFFER_SIZE; i++ )
		{
			if( receiveBuffer[ i ] == 'e' )
			{
				end_of_string = i;
			}
		}
		
		for( uint8_t i = 0; i < receiveTail; i++ )
		{
			if( receiveBuffer[ i ] == 'e' )
			{
				end_of_string = i;
			}
		}				
	}		
	
	// we've got string?
	if( !end_of_string )	
		return 0;
	
	// clear command buffer
	for( uint8_t i = 0; i < RECEIVE_BUFFER_SIZE; i++ )
	{
		commandBuffer[ i ] = 0;
	}	
	
	// copy command from serial to the command buffer
	uint8_t n = 0;
	
	while( 1 )
	{
		commandBuffer[ n ] = serial_get( );
		

		
		if( commandBuffer[ n ] == 0 || n >= RECEIVE_BUFFER_SIZE )
			break;
			
			if( commandBuffer[ n ] == 'e' )
			return &commandBuffer[ 0 ];
			
			serial_send( commandBuffer[ n ] );
					
		n++;
	}
	
	if( n > 0 )
		return &commandBuffer[ 0 ];
	else
		return 0 ;*/
}

/* 
typical format of 'str' is

LITERAL_OF_COMMAND ARG1 ARG2 ... ARG_n

i.e.

L 100 20 30 600

adds line to queue

LITERAL_OF_COMMAND can be:
	L	- add line to queue. args - x0, y0, x1, y1
	U	- move up instrument. args - steps
	D	- move down instrument. args - steps
*/

void exec_string( char * str )
{
	if( !str )
		return;
		
	while( !commandDone ) { }; // wait until last command becomes done
	
	for( uint8_t i = 0 ; i < 127; i++ )		
		serial_send( str[ i ] );
		
	char command		= 0;
	const char * delim	= " ";
	char * token		= 0;
	uint8_t argNum		= 0;
	int16_t args[ 4 ]	= { 0 };

	// get first token
	token = strtok( str, delim );

	if( !token )
		return;

	// get command literal
	command = token[ 0 ];

	// get args of command
	do
	{
		token = strtok( 0, delim );

		if( token )
		{
			args[ argNum++ ] = atoi( token );
		}
	
	} while( token );
	
	// execute command
	switch( command )
	{
		// add line to queue
		case 'L':
		{
			//lq_add_line( args[ 0 ], args[ 1 ], args[ 2 ], args[ 3 ] );
			line( args[ 0 ], args[ 1 ], args[ 2 ], args[ 3 ] );
			
			drawLineMode = 1;
			commandDone  = 0;
			
			break;	
		}
		
		// up\down intrument
		case 'U':
		case 'D':
		{
			if( command == 'U' )
			{
				stepper_set_direction( g_steppers[ Z_STEPPER ],  1 );
			}				
			if( command == 'D' )
			{
				stepper_set_direction( g_steppers[ Z_STEPPER ], -1 );	
			}				
							
			stepper_do_steps( g_steppers[ Z_STEPPER ], args[ 0 ] );
			
			while( !stepper_is_ready( g_steppers[ Z_STEPPER ] ) ) // wait until stepper done all steps
			{
				
			}; 
				
			break;
		}					
	}	
}

// draw line
ISR( TIMER0_OVF_vect )
{
	if( timer > 14 )
	{		
		if( drawLineMode ) // use dependent motor control
		{
			if( x0 != x1 || y0 != y1 )
			{
				e2 = err * 2;
				
				if ( e2 > -dy )
				{
					err -= dy;
					x0  += sx;
					g_x += sx;
					
					stepper_do_steps( g_steppers[ X_STEPPER ], 1 );
					
					stepper_update( g_steppers[ X_STEPPER ] );	
				}
				
				if ( e2 < dx )
				{
					err += dx;
					y0  += sy;
					g_y += sy;
					
					stepper_do_steps( g_steppers[ Y_STEPPER ], 1 );
					
					stepper_update( g_steppers[ Y_STEPPER ] );	
				}
			}
			else
			{
				drawLineMode = 0;
				commandDone   = 1;
			}
		}
		else
		{
			for( uint8_t i = 0; i < STEPPERS_COUNT; i++ )
			{
				if( g_steppers[ i ] )
				stepper_update( g_steppers[ i ] );
			}
		}
		
		timer = 0;
	}
	
	timer++;
}

int main(void)
{
	DDRA  = 0xFF;
	PORTA = 0x00;
	PORTC = 0x00;
	DDRC  = 0xFF;
	
	PORTB = 0xFF;
	DDRB  = 0x00;
	
	init_stepper_control_timer();
	serial_init();
	
	sei();
		
	stepper_init( &g_x_stepper, &PORTA, 0  );	
	g_steppers[ X_STEPPER ] = &g_x_stepper;
	
	stepper_init( &g_y_stepper, &PORTA, SF_USE_HIGH_4_BITS );
	g_steppers[ Y_STEPPER ] = &g_y_stepper;	
	
	stepper_init( &g_z_stepper, &PORTC, SF_USE_HIGH_4_BITS );
	g_steppers[ Z_STEPPER ] = &g_z_stepper;	

	uint8_t started = 0x00;
	
    while( 1 )
    {
		exec_string( receive_buffer_get_string() );
	    if( commandDone )
	    {
			
			
			/*
		    if( started )
		    {	
				strcpy( commandBuffer, "U 200" );
				exec_string( commandBuffer );				
				strcpy( commandBuffer, "L 0 0 200 300\0" );
				exec_string( commandBuffer );
				strcpy( commandBuffer, "L 200 300 400 0\0" );
				exec_string( commandBuffer );
				strcpy( commandBuffer, "L 400 0 0 200\0" );
				exec_string( commandBuffer );
				strcpy( commandBuffer, "L 0 200 400 200\0" );
				exec_string( commandBuffer );	
				strcpy( commandBuffer, "L 400 200 0 0\0" );
				exec_string( commandBuffer );
				strcpy( commandBuffer, "D 200" );
				exec_string( commandBuffer );											

				started = 0x00;	
		    }
			else
			{
				if( !( PINB & ( 1 << 4 ) ) )
				{
					if( commandDone == 1 )
						started = 0xFF;
				}			    
		    }*/
			
			// keyboard control
			if( !( PINB & ( 1 << 0 ) ) ) 
			{
				stepper_set_direction( g_steppers[ X_STEPPER ], 1 );
				stepper_do_steps( g_steppers[ X_STEPPER ], 2 );
			}
			if( !( PINB & ( 1 << 1 ) ) )
			{
				stepper_set_direction( g_steppers[ X_STEPPER ], -1 );
				stepper_do_steps( g_steppers[ X_STEPPER ], 2 );
			}
			if( !( PINB & ( 1 << 2 ) ) )
			{
				stepper_set_direction( g_steppers[ Z_STEPPER ], 1 );
				stepper_do_steps( g_steppers[ Z_STEPPER ], 2 );
			}
			if( !( PINB & ( 1 << 3 ) ) )
			{
				stepper_set_direction( g_steppers[ Z_STEPPER ], -1 );
				stepper_do_steps( g_steppers[ Z_STEPPER ], 2 );
			}		    
	    }		
    }
}