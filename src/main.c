

// define F_CPU here as the FLL frequency
// otherwise delay_ms(n) functions try to dynamically 
// get the cpu frequency which ends up blowing up the 
// stack and corrupting the 'n' 
#define F_CPU 48000000UL

#include <asf.h>


extern volatile bool DEVICE_ENUMERATED_RUNNING; 
#define N_CTRLS   8

#define PITCHBEND_CTRL_INPUT 0
uint8_t controller_value[N_CTRLS];

struct adc_module adc_instance;

void configure_adc(void);
uint16_t adc_read_value(const uint8_t input_channel);
void scan_controls(bool output_changes);


void
configure_adc(void)
{
  struct adc_config config;
  adc_get_config_defaults(&config);
  config.reference_compensation_enable = false;
  // REF max voltage is VDDANA-.6, // this is pin 20  
  config.reference = ADC_REFERENCE_AREFA;   
  // INTVCC0 is 1/1.48 VDDANA   INTVCC1 is 1/2 VDDANA (but only for VDDANA>2.0)
  //config.reference = ADC_REFERENCE_INTVCC1;  
  // 0-63 ... controls the length of time the sampling is done
  // and controls the input impedence 
  config.sample_length = 63;     
  config.resolution = ADC_RESOLUTION_12BIT;
  config.divide_result = ADC_DIVIDE_RESULT_128;
  config.accumulate_samples = ADC_ACCUMULATE_SAMPLES_128;
  config.pin_scan.inputs_to_scan = 0;
  config.pin_scan.offset_start_scan = 0;
  // clock should be between 30k and 2.1MHz
  config.clock_source = GCLK_GENERATOR_2;  
  
  while (adc_init(&adc_instance, ADC, &config) != STATUS_OK);
  while (adc_enable(&adc_instance) != STATUS_OK);
  
}

// ADC_POSITIVE_INPUT_PIN2 is setup to be the VREFB
uint16_t
adc_read_value(const uint8_t input_channel) {
  enum status_code status;
  enum adc_positive_input input; 
  uint16_t result;
  int retries = 5000;
  
  switch (input_channel) { 
  case 0:
	input = ADC_POSITIVE_INPUT_PIN12;
	break;
  case 1: 
    input = ADC_POSITIVE_INPUT_PIN13;
    break;
  case 2:
    input = ADC_POSITIVE_INPUT_PIN14;
    break;
  case 3:
    input = ADC_POSITIVE_INPUT_PIN15;
    break;
  case 4:
    input = ADC_POSITIVE_INPUT_PIN5;
    break;
  case 5:
    input = ADC_POSITIVE_INPUT_PIN4;
    break; 
  case 6:
    input = ADC_POSITIVE_INPUT_PIN3;
	break; 
  case 7:
    input = ADC_POSITIVE_INPUT_PIN2;
	break;
  case 8:
	input = ADC_POSITIVE_INPUT_PIN15;
    break; 
  case 9:
    input = ADC_POSITIVE_INPUT_PIN9;
	break;
  case 10: 
	input = ADC_POSITIVE_INPUT_PIN10;
	break;
  case 11:
	input = ADC_POSITIVE_INPUT_PIN11;
	break;
  default:
    return 0xffff;
  }
  
  
  adc_set_positive_input(&adc_instance, input);
  adc_start_conversion(&adc_instance);
  do {
    status = adc_read(&adc_instance, &result);
    retries--;
  } while (status == STATUS_BUSY && retries > 0);
  if (status != STATUS_OK) {
    return 0xffff;
  }
  return result;
}




uint16_t current_pitchbend_value = 0x2000;
uint16_t last_sent_pitchbend_value = 0xffff;

// the end points and mid point are of particular interest for the sake of tuning
// so make sure we cover the whole region even though we may not get the adc values
// < 100 or > 16200

inline uint16_t fixup_pitchbend_value(uint16_t value) {
	
	int32_t temp = value; 
	temp -= 8192;    // center around 0
	temp *= 1008; // ideally we want to multiply by ~1.012358648 to scale just slightly bigger use 1012/1000
	temp /= 1000;
	temp += 8192;    // put it back to the normal center

	if (temp < 0) {
		temp = 0;
	} else if (temp > 16383) {
		temp = 16383;
	} else if ((temp > 8192-100) && (temp < 8192+100)) {
		temp = 8192;
	}
	return (uint16_t)temp;
}


inline void handle_pitchbend(bool output_changes, int i, uint16_t value) {
    
    // this is signed so we can deal with the 0 bin appropriately 
	
	//	value = (value>>5)<<7        ;
	value = value << 2; 

	uint16_t fixedup_pitchbend_value = 0;

	bool controller_changed = abs(value - current_pitchbend_value) > 0x1f;


    if (controller_changed) {
		current_pitchbend_value += (value - current_pitchbend_value)/4;
		
	  // we clamp the values and put a deadband in the center
	  // so we might have already sent a 0 or 0x4000 or 0x2000 even though
	  // the raw value changed
	  fixedup_pitchbend_value = fixup_pitchbend_value(current_pitchbend_value);
      if (output_changes && fixedup_pitchbend_value != last_sent_pitchbend_value) {
        // only record the value, if we actually got it in the queue
		if (enqueue_ctrl(CTRL_PITCHBEND, fixedup_pitchbend_value)) {
			last_sent_pitchbend_value = fixedup_pitchbend_value;
		}
      } else {
        //current_pitchbend_value = value;		
      } 
	}
}

// scan the controls, but only send the changes to the midi out if 
// output_changes is true
// we want to apply some hysteresis to the value change so:

// there are 0-127 bins representing each control value
// BIN2RAW converts the bin number to the raw adc value of the middle of a bin
#define HALFBIN_SIZE   (1<<4)
#define GUARD_SIZE     (1<<3)
#define BIN2RAW(x)     (((x)<<5) + HALFBIN_SIZE)


inline void handle_ctrl_value(bool output_changes, int i, uint16_t value) {
    uint8_t res = (value >> 5);  // scale from 0-0x0fff to 0-0x7f
    
    // this is signed so we can deal with the 0 bin appropriately 
    int raw_bin_middle = (int)BIN2RAW(controller_value[i]);
    int raw_bin_high = raw_bin_middle+HALFBIN_SIZE+GUARD_SIZE;
    int raw_bin_low  = raw_bin_middle-HALFBIN_SIZE-GUARD_SIZE;
    bool controller_changed = (value > raw_bin_high) || (value < raw_bin_low);
    
    if (controller_changed) {
      if (output_changes) {
        // only record the value, if we actually got it in the queue
		if (enqueue_ctrl(i, res)) {
			controller_value [i] = res;
		}
      } else {
        controller_value [i] = res;		
      } 
	}
}

void scan_controls(bool output_changes) {
  for (int i = 0; i < N_CTRLS; i++) {
    uint16_t v = adc_read_value(i);
    if (v == 0xffff) {
      continue; // error during adc_read
    }
	if (i == PITCHBEND_CTRL_INPUT) {
		handle_pitchbend(output_changes, i, v);   
	} else {  
		handle_ctrl_value(output_changes, i, v);
	}
  } // for
}


int main (void)
{
  DEVICE_ENUMERATED_RUNNING = false;
  system_init();
  irq_initialize_vectors();
  cpu_irq_enable();
  sleepmgr_init();
  udc_start();
  configure_adc();
  scan_controls(false);

  while (1) {

	while (DEVICE_ENUMERATED_RUNNING) { 
	    scan_controls(true);
		// is this a good tradeoff...we don't want to inundate the
		// host with events
		delay_ms(5);
	}
	sleepmgr_sleep(SLEEPMGR_IDLE_0);
  }
  

}
