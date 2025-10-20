#include "cJSON.h"
#include <string>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "reflect.hpp" 

#define LOG_TAG "realtimeapi"
extern const uint8_t _binary_oai_instructions_txt_start[] asm("_binary_oai_instructions_txt_start");
extern const uint8_t _binary_oai_instructions_txt_end[] asm("_binary_oai_instructions_txt_end");

// sandhya's preferred standard lights
/* Found using lifx lan:attr IP ADDRESS GetColor 
{"brightness": 1.0, "hue": 41.46, "kelvin": 3500, "label": "lifex1", "power": 65535, "reserved6": "0000", "reserved7": "0000000000000000", "saturation": 0.131}*/
const uint16_t DEFAULT_HUE = 7547;
const uint16_t DEFAULT_SATURATION = 8585;
const uint16_t DEFAULT_BRIGHTNESS = 65535;
const uint16_t DEFAULT_KELVIN = 3500;

void set_default_light_state(uint32_t duration) {
    ESP_LOGI(LOG_TAG, "Returning to default lights");
    send_lifx_set_power(true, duration);
    send_lifx_set_color(DEFAULT_HUE, DEFAULT_SATURATION, DEFAULT_BRIGHTNESS, DEFAULT_KELVIN, duration);
}

typedef struct {
    uint16_t hue;
    uint16_t saturation;
} firecracker_params_t;

void firecracker_task(void *params) {
    firecracker_params_t *p = (firecracker_params_t *)params;

    send_lifx_set_power(false, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    send_lifx_set_power(true, 100); 

    ESP_LOGI(LOG_TAG, "Firecracker Task: 1. Launch (triangle)...");
    //send_lifx_set_waveform(true,p->hue, p->saturation,65535,3500,900,1.0f,0,3);
    //send_lifx_set_waveform(true,10923, 65535,65535,3500,900,1.0f,0,3);
    send_lifx_set_waveform(true,0, 0,20000,4000,2000,1.5f,0,3);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(LOG_TAG, "Firecracker Task: 2. Explosion (pulse)!");
    send_lifx_set_waveform(true,p->hue,p->saturation,65535,3500,200,2.0f,0,4);
    vTaskDelay(pdMS_TO_TICKS(250));

    uint16_t ember_h = (uint16_t)((p->hue + (uint16_t)(65535.0f * 15.0f / 360.0f)) % 65535);
    uint16_t ember_s1 = (uint16_t)((float)p->saturation * 0.80f);
    uint16_t ember_s2 = (uint16_t)((float)p->saturation * 0.90f);
    uint16_t ember_s3 = (uint16_t)((float)p->saturation * 0.50f);

    ESP_LOGI(LOG_TAG, "Firecracker embers started ");
    send_lifx_set_waveform(false,ember_h,ember_s1,35000, 3500, 600, 1.0f, 0,2);
    vTaskDelay(pdMS_TO_TICKS(1100));

    ESP_LOGI(LOG_TAG, "embers next");
    send_lifx_set_waveform(true,ember_h,ember_s2,28000,3500,150,6.0f,16383,4);
    vTaskDelay(pdMS_TO_TICKS(1700));

    ESP_LOGI(LOG_TAG, "fading embers");
    send_lifx_set_waveform(false,ember_h,ember_s3,8000,3500,1800,1.0f,0,2);
    vTaskDelay(pdMS_TO_TICKS(3200));

    set_default_light_state(1000);

    free(p);
    vTaskDelete(NULL);
}

void diwali_lights_task(void *params) {
    ESP_LOGI(LOG_TAG, "Diwali Lights Task: Starting...");
    
    vTaskDelay(pdMS_TO_TICKS(1200));

    bool transient = true;        
    uint16_t hue = 8192;          
    uint16_t saturation = 65535;  
    uint16_t brightness = 65535;  
    uint16_t kelvin = 3500;
    uint32_t period = 2000;       
    float cycles = 3600.0f;      
    
    int16_t skew_ratio = 16383;
    uint8_t waveform = 4;

    ESP_LOGI(LOG_TAG, "Diwali Lights have started.");
    send_lifx_set_waveform(transient, hue, saturation, brightness, kelvin, period, cycles, skew_ratio, waveform);
    
    vTaskDelete(NULL);
  }

  void diwali_second_sequence_task(void *params) {
    ESP_LOGI(LOG_TAG, "Diwali Second one Starting...");

    //set_default_light_state(1000);
    vTaskDelay(pdMS_TO_TICKS(500));

    uint16_t hues[] = {21845, 10922, 5461, 0}; 
    uint16_t saturation = 65535; 
    uint16_t brightness = 60000; 
    uint16_t kelvin = 3500;
    for (int i = 0; i < 900; i++) {
        // Loop through the four  colors
        for (uint16_t hue : hues) {
            send_lifx_set_color(hue, saturation, brightness, kelvin, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
      }
    set_default_light_state(1000);
    vTaskDelete(NULL);
  }

void reflect_start_firecracker_task(uint16_t hue, uint16_t saturation) {
    firecracker_params_t *params = (firecracker_params_t *)malloc(sizeof(firecracker_params_t));
    params->hue = hue;
    params->saturation = saturation;

    xTaskCreate(firecracker_task, "firecracker_task", 4096, params, 5, NULL);
}

void set_required_parameters(cJSON *parameters,
                             std::vector<std::string> params) {
  cJSON *required = cJSON_AddArrayToObject(parameters, "required");
  assert(required != nullptr);

  for (auto param : params) {
    auto val = cJSON_CreateString(param.c_str());
    assert(val != nullptr);

    assert(cJSON_AddItemToArray(required, val));
  }
}

void add_number_parameter(cJSON *properties, std::string name,
                          std::string description, int defaultValue,
                          int minimum, int maximum) {
  cJSON *duration = cJSON_AddObjectToObject(properties, name.c_str());
  assert(duration != nullptr);

  assert(cJSON_AddStringToObject(duration, "type", "number") != nullptr);
  assert(cJSON_AddStringToObject(duration, "description",
                                 description.c_str()) != nullptr);
  assert(cJSON_AddNumberToObject(duration, "default", defaultValue) != nullptr);
  assert(cJSON_AddNumberToObject(duration, "minimum", minimum) != nullptr);
  assert(cJSON_AddNumberToObject(duration, "default", maximum) != nullptr);
}

void add_set_light_power(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "set_light_power") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "LAN SetLightPower (117). Turn on/off with optional fade.") !=
         nullptr);

  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);

  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr);

  auto on = cJSON_AddObjectToObject(properties, "on");
  assert(on != nullptr);
  assert(cJSON_AddStringToObject(on, "type", "boolean") != nullptr);
  assert(cJSON_AddStringToObject(on, "description", "true=on, false=off") !=
         nullptr);

  add_number_parameter(properties, "duration",
                       "duration of transition in milliseconds", 0, 0, 1000);

  set_required_parameters(parameters,
                          std::vector<std::string>{"on", "duration"});
  assert(cJSON_AddItemToArray(tools, tool));
}

void add_set_color(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "set_color") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "LAN SetColor (102). Set HSBK for whole device.") != nullptr);

  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);

  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr);

  add_number_parameter(properties, "hue", "", 0, 0, 65535);
  add_number_parameter(properties, "saturation", "", 0, 0, 65535);
  add_number_parameter(properties, "brightness", "", 0, 0, 65535);
  add_number_parameter(properties, "kelvin", "", 0, 0, 65535);
  add_number_parameter(properties, "duration", "", 0, 0, 4294967295);

  set_required_parameters(
      parameters, std::vector<std::string>{"hue", "saturation", "brightness",
                                           "kelvin", "duration"});
  assert(cJSON_AddItemToArray(tools, tool));
}

void add_set_waveform(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "set_waveform") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "LAN SetWaveform (103). Modulate HSBK values over time with a waveform.") != nullptr);

  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);

  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr);

  auto transient = cJSON_AddObjectToObject(properties, "transient");
  assert(transient != nullptr);
  assert(cJSON_AddStringToObject(transient, "type", "boolean") != nullptr);
  assert(cJSON_AddStringToObject(transient, "description", "If true, light returns to original color after effect. SINE/TRIANGLE always return.") != nullptr);

  add_number_parameter(properties, "hue", "Target hue", 0, 0, 65535);
  add_number_parameter(properties, "saturation", "Target saturation", 0, 0, 65535);
  add_number_parameter(properties, "brightness", "Target brightness", 0, 0, 65535);
  add_number_parameter(properties, "kelvin", "Target kelvin", 3500, 1500, 9000);
  add_number_parameter(properties, "period", "Duration of one cycle in milliseconds", 1000, 0, 4294967295);
  add_number_parameter(properties, "cycles", "Number of cycles to repeat", 1, 0, 100);
  
  add_number_parameter(properties, "skew_ratio", "For PULSE only. Defines duty cycle. Scaled from 0 to 1 as -32768 to 32767.", 0, -32768, 32767);
  
  add_number_parameter(properties, "waveform", "Shape of the wave. 0:SAW, 1:SINE, 2:HALF_SINE, 3:TRIANGLE, 4:PULSE", 1, 0, 4);
  
  set_required_parameters(
      parameters, std::vector<std::string>{"transient", "hue", "saturation", "brightness", "kelvin", "period", "cycles", "waveform"});

  assert(cJSON_AddItemToArray(tools, tool));
}

void add_run_firecracker(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "run_firecracker") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "Runs a pre-programmed firecracker lighting sequence on the device.") != nullptr);
  
  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);

  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr);

  add_number_parameter(properties, "hue", "Optional hue for the firecracker launch color.", 10923, 0, 65535);
  add_number_parameter(properties, "saturation", "Optional saturation for the launch color.", 65535, 0, 65535);
  
  assert(cJSON_AddItemToArray(tools, tool));
}

void add_run_diwali_lights(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "run_diwali_lights") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "Golden sparkling 'Diwali' lighting effect for 60 minutes or until when another commd is given.") != nullptr);
  
  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);
  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr);

  assert(cJSON_AddItemToArray(tools, tool));
}

void add_run_diwali_second_sequence(cJSON *tools) {
  auto tool = cJSON_CreateObject();
  assert(tool != nullptr);

  assert(cJSON_AddStringToObject(tool, "type", "function") != nullptr);
  assert(cJSON_AddStringToObject(tool, "name", "run_diwali_second_sequence") != nullptr);
  assert(cJSON_AddStringToObject(
             tool, "description",
             "Cycles through green, yellow, orange, and red lights for one hour.") != nullptr);
  
  auto parameters = cJSON_CreateObject();
  assert(parameters != nullptr);
  assert(cJSON_AddItemToObject(tool, "parameters", parameters));
  assert(cJSON_AddStringToObject(parameters, "type", "object") != nullptr);
  auto properties = cJSON_AddObjectToObject(parameters, "properties");
  assert(properties != nullptr); 

  assert(cJSON_AddItemToArray(tools, tool));
}

void send_session_update(PeerConnection *peer_connection) {
  std:: string kLunaInstructions(
    reinterpret_cast<const char*>(_binary_oai_instructions_txt_start),_binary_oai_instructions_txt_end - _binary_oai_instructions_txt_start);
  
  auto root = cJSON_CreateObject();
  assert(root != nullptr);

  assert(cJSON_AddStringToObject(root, "type", "session.update") != nullptr);

  auto session = cJSON_CreateObject();
  assert(session != nullptr);

  assert(cJSON_AddStringToObject(session, "instructions", kLunaInstructions.c_str()) != nullptr);
  assert(cJSON_AddStringToObject(session, "type", "realtime") != nullptr);

  auto tools = cJSON_AddArrayToObject(session, "tools");
  assert(tools != nullptr);

  add_set_light_power(tools);
  add_set_color(tools);
  add_set_waveform(tools);
  add_run_firecracker(tools);
  add_run_diwali_lights(tools);
  add_run_diwali_second_sequence(tools); 

  assert(cJSON_AddItemToObject(root, "session", session));

  auto serialized = cJSON_PrintUnformatted(root);
  assert(serialized != nullptr);

  peer_connection_datachannel_send(peer_connection, serialized, strlen(serialized));
  cJSON_free(serialized);
  cJSON_Delete(root);
}

void realtimeapi_parse_incoming(char *msg) {
  // Large inbound messages get chunked (and fail to parse)
  auto root = cJSON_Parse(msg);
  if (root == nullptr) {
    return;
  }

  auto type_item = cJSON_GetObjectItem(root, "type");
  if (type_item == nullptr) {
    return;
  }

  if (strcmp(type_item->valuestring, "response.function_call_arguments.done") !=
      0) {
    return;
  }

  auto argsString = cJSON_GetObjectItem(root, "arguments");
  assert(cJSON_IsString(argsString));

  auto args = cJSON_Parse(argsString->valuestring);
  assert(cJSON_IsObject(args));

  uint16_t hue = 0;
  uint16_t saturation = 0;
  uint16_t brightness = 0;
  uint16_t kelvin = 0;
  uint32_t duration = 0;
  bool on = false;

  auto hueObj = cJSON_GetObjectItem(args, "hue");
  if (hueObj != nullptr) {
    hue = hueObj->valueint;
  }

  auto saturationObj = cJSON_GetObjectItem(args, "saturation");
  if (saturationObj != nullptr) {
    saturation = saturationObj->valueint;
  }

  auto brightnessObj = cJSON_GetObjectItem(args, "brightness");
  if (brightnessObj != nullptr) {
    brightness = brightnessObj->valueint;
  }

  auto kelvinObj = cJSON_GetObjectItem(args, "kelvin");
  if (kelvinObj != nullptr) {
    kelvin = kelvinObj->valueint;
  }

  auto durationObj = cJSON_GetObjectItem(args, "duration");
  if (durationObj != nullptr) {
    duration = durationObj->valueint;
  }

  auto onObj = cJSON_GetObjectItem(args, "on");
  if (onObj != nullptr) {
    on = onObj->type == cJSON_True;
  }

  auto output_name_item = cJSON_GetObjectItem(root, "name");
  assert(cJSON_IsString(output_name_item));

  if (strcmp(output_name_item->valuestring, "set_color") == 0) {
    ESP_LOGI(LOG_TAG,
             "set_color hue(%d) saturation(%d) brightness(%d) kelvin(%d) "
             "duration(%d)",
             hue, saturation, brightness, kelvin, duration);
    send_lifx_set_color(hue, saturation, brightness, kelvin, duration);
  } else if (strcmp(output_name_item->valuestring, "set_light_power") == 0) {
    ESP_LOGI(LOG_TAG, "set_light_power on(%d) duration(%d)", on, duration);
    send_lifx_set_power(on, duration);
  } else if (strcmp(output_name_item->valuestring, "set_waveform")==0){
    ESP_LOGI(LOG_TAG, "set_waveform call received");

    bool transient = false;
    auto transientObj = cJSON_GetObjectItem(args, "transient");
    if (transientObj != nullptr) {
      transient = cJSON_IsTrue(transientObj);
    }

    uint16_t hue = 0;
    auto hueObj = cJSON_GetObjectItem(args, "hue");
    if (hueObj != nullptr) {
      hue = hueObj->valueint;
    }
    
    uint16_t saturation = 0;
    auto saturationObj = cJSON_GetObjectItem(args, "saturation");
    if (saturationObj != nullptr) {
      saturation = saturationObj->valueint;
    }
    
    uint16_t brightness = 0;
    auto brightnessObj = cJSON_GetObjectItem(args, "brightness");
    if (brightnessObj != nullptr) {
      brightness = brightnessObj->valueint;
    }
    
    uint16_t kelvin = 3500; 
    auto kelvinObj = cJSON_GetObjectItem(args, "kelvin");
    if (kelvinObj != nullptr) {
      kelvin = kelvinObj->valueint;
    }
    
    uint32_t period = 1000; 
    auto periodObj = cJSON_GetObjectItem(args, "period");
    if (periodObj != nullptr) {
      period = periodObj->valueint;
    }
    
    float cycles = 1.0f; 
    auto cyclesObj = cJSON_GetObjectItem(args, "cycles");
    if (cyclesObj != nullptr) {
      cycles = (float)cyclesObj->valuedouble;
    }

    int16_t skew_ratio = 0; 
    auto skew_ratioObj = cJSON_GetObjectItem(args, "skew_ratio");
    if (skew_ratioObj != nullptr) {
      skew_ratio = skew_ratioObj->valueint;
    }
    
    uint8_t waveform = 1;
    auto waveformObj = cJSON_GetObjectItem(args, "waveform");
    if (waveformObj != nullptr) {
      waveform = waveformObj->valueint;
    }

    ESP_LOGI(LOG_TAG, "Executing waveform: type(%d) period(%d) cycles(%.1f)", waveform, period, cycles);

    send_lifx_set_waveform(transient, hue, saturation, brightness, kelvin, period, cycles, skew_ratio, waveform);
  } else if (strcmp(output_name_item->valuestring, "run_firecracker") == 0) {
    ESP_LOGI(LOG_TAG, "run_firecracker call received");

  
    uint16_t hue = 10923; 
    uint16_t saturation = 65535; 

    auto hueObj = cJSON_GetObjectItem(args, "hue");
    if (hueObj != nullptr) {
      hue = hueObj->valueint;
    }

    auto saturationObj = cJSON_GetObjectItem(args, "saturation");
    if (saturationObj != nullptr) {
      saturation = saturationObj->valueint;
    }

    ESP_LOGI(LOG_TAG, "Starting firecracker task with H:%d, S:%d", hue, saturation);
    reflect_start_firecracker_task(hue, saturation);
  } else if (strcmp(output_name_item->valuestring, "run_diwali_lights") == 0) {
    ESP_LOGI(LOG_TAG, "run_diwali_lights call received");
    xTaskCreate(diwali_lights_task, "diwali_lights_task", 4096, NULL, 5, NULL);
  } else if (strcmp(output_name_item->valuestring, "run_diwali_second_sequence") == 0) {
    ESP_LOGI(LOG_TAG, "run_diwali_second_sequence call received");
    xTaskCreate(diwali_second_sequence_task, "diwali_second_sequence_task", 4096, NULL, 5, NULL);
  }

  cJSON_Delete(args);
  cJSON_Delete(root);
}
