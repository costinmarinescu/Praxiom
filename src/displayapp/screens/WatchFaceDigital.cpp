// IMPROVED VERSION - WatchFaceDigital.cpp
// This version forces immediate display update on boot for easier testing

// ... (keep all the includes and constructor the same) ...

void WatchFaceDigital::Refresh() {
  statusIcons.Update();

  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date,
                            "%s %d %s %d",
                            dateTimeController.DayOfWeekShortToString(),
                            day,
                            dateTimeController.MonthShortToString(),
                            year);
      lv_obj_realign(label_date);
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "");
    }
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
  }

  // ✅ IMPROVED: Check if Bio-Age was updated from mobile app via BLE
  uint32_t bleAge = praxiomService.GetBasePraxiomAge();
  if (bleAge >= 18 && bleAge <= 120 && bleAge != static_cast<uint32_t>(basePraxiomAge)) {
    basePraxiomAge = static_cast<int>(bleAge);
    lastSyncTime = dateTimeController.CurrentDateTime().time_since_epoch().count();
    
    // INSTANT UPDATE with LARGE font for number
    int praxiomAge = GetCurrentPraxiomAge();
    lv_obj_set_style_local_text_font(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                      LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
    lv_label_set_text_fmt(labelPraxiomAgeNumber, "%d", praxiomAge);
    lv_color_t ageColor = GetPraxiomAgeColor(praxiomAge, basePraxiomAge);
    lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                      LV_STATE_DEFAULT, ageColor);
    lv_obj_realign(labelPraxiomAgeNumber);
  }

  // ✅ IMPROVED: Force update on first call, then update every second
  // This ensures the display shows current value immediately
  static bool firstRun = true;
  static uint8_t lastSecond = 255;  // Invalid value to force first update
  uint8_t currentSecond = dateTimeController.Seconds();
  
  // Update on first run OR every second (for immediate feedback)
  if (firstRun || currentSecond != lastSecond) {
    if (basePraxiomAge > 0) {
      // We have valid data - display it with LARGE font
      int praxiomAge = GetCurrentPraxiomAge();
      lv_obj_set_style_local_text_font(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                        LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
      lv_label_set_text_fmt(labelPraxiomAgeNumber, "%d", praxiomAge);
      
      lv_color_t ageColor = GetPraxiomAgeColor(praxiomAge, basePraxiomAge);
      lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                        LV_STATE_DEFAULT, ageColor);
    } else {
      // No data yet - show placeholder with SMALL font
      lv_obj_set_style_local_text_font(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                        LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
      lv_label_set_text_static(labelPraxiomAgeNumber, "---");
      lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, 
                                        LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    }
    
    lv_obj_realign(labelPraxiomAgeNumber);
    lastSecond = currentSecond;
    firstRun = false;
  }
}
