
// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void printCurrentConfig() {
  const FataMorganaMapping& mapping = client.getMapping();

  Serial.println(F("\n--- Current Transforms ---"));
  Serial.printf("Rotation: %u° (%s)\n",
                mapping.rotation * 90,
                mapping.rotation == 0 ? "0°" :
                mapping.rotation == 1 ? "90° CW" :
                mapping.rotation == 2 ? "180°" : "270° CW");
  Serial.printf("Flip X (Horizontal): %s\n", mapping.flipX ? "ON" : "OFF");
  Serial.printf("Flip Y (Vertical):   %s\n", mapping.flipY ? "ON" : "OFF");
  Serial.printf("Flip Z (Diagonal):   %s\n", mapping.flipZ ? "ON" : "OFF");
  Serial.printf("Serpentine: %s\n", fatamorgana_serpentineModeName(mapping.serpentine));
  Serial.println();
}

void setRotation(uint8_t rotation) {
  Serial.printf("Setting rotation to %u°\n", rotation * 90);
  currentRotation = rotation;
  client.setRotation(rotation);
  printCurrentConfig();
}

void toggleFlipX() {
  currentFlipX = !currentFlipX;
  Serial.printf("Flip X: %s\n", currentFlipX ? "ON" : "OFF");
  client.setFlip(currentFlipX, currentFlipY, currentFlipZ);
  printCurrentConfig();
}

void toggleFlipY() {
  currentFlipY = !currentFlipY;
  Serial.printf("Flip Y: %s\n", currentFlipY ? "ON" : "OFF");
  client.setFlip(currentFlipX, currentFlipY, currentFlipZ);
  printCurrentConfig();
}

void toggleFlipZ() {
  currentFlipZ = !currentFlipZ;
  Serial.printf("Flip Z: %s\n", currentFlipZ ? "ON" : "OFF");
  client.setFlip(currentFlipX, currentFlipY, currentFlipZ);
  printCurrentConfig();
}

void cycleSerpentine() {
  currentSerpentine = (currentSerpentine + 1) % 3;
  Serial.printf("Serpentine: %s\n", fatamorgana_serpentineModeName(currentSerpentine));
  client.setSerpentine(currentSerpentine);
  printCurrentConfig();
}

void resetTransforms() {
  Serial.println(F("Resetting all transforms..."));
  currentRotation = 0;
  currentFlipX = false;
  currentFlipY = false;
  currentFlipZ = false;
  currentSerpentine = SERPENTINE_NONE;

  client.setRotation(0);
  client.setFlip(false, false, false);
  client.setSerpentine(SERPENTINE_NONE);
  printCurrentConfig();
}

void printHelp() {
  Serial.println(F("\n=== FataMorgana Transform Demo ==="));
  Serial.println(F("Rotation Commands:"));
  Serial.println(F("  0 - No rotation (0°)"));
  Serial.println(F("  1 - 90° clockwise"));
  Serial.println(F("  2 - 180°"));
  Serial.println(F("  3 - 270° clockwise (90° counter-clockwise)"));
  Serial.println(F("\nFlip Commands:"));
  Serial.println(F("  x - Toggle Flip X (horizontal mirror)"));
  Serial.println(F("  y - Toggle Flip Y (vertical mirror)"));
  Serial.println(F("  z - Toggle Flip Z (diagonal/transpose)"));
  Serial.println(F("\nLayout Commands:"));
  Serial.println(F("  s - Cycle serpentine mode (none/horizontal/vertical)"));
  Serial.println(F("\nOther Commands:"));
  Serial.println(F("  r - Reset all transforms"));
  Serial.println(F("  ? - Show this help"));
  Serial.println(F("==================================\n"));
}

void printUseCases() {
  Serial.println(F("\n=== Common Use Cases ==="));
  Serial.println(F("\n1. LED Matrix Mounted Upside Down:"));
  Serial.println(F("   Command: 2 (180° rotation)"));
  Serial.println(F("\n2. Standard Horizontal Serpentine Wiring:"));
  Serial.println(F("   Command: s (until 'horizontal')"));
  Serial.println(F("\n3. Vertical LED Strip (top-to-bottom):"));
  Serial.println(F("   Command: s (until 'vertical')"));
  Serial.println(F("\n4. Matrix Rotated 90° with Mirrored Columns:"));
  Serial.println(F("   Commands: 1, then x"));
  Serial.println(F("\n========================\n"));
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println();

  printHelp();
  printUseCases();

  // Connect to WiFi
  Serial.print(F("Connecting to WiFi"));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());

  // Initialize FataMorgana
  if (!client.begin()) {
    Serial.println(F("ERROR: Failed to initialize!"));
    while (1) delay(1000);
  }

  // Configure rectangle mapping (full matrix)
  client.setRectangle(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT);
  client.setBrightness(80);

  printCurrentConfig();

  Serial.println(F("Ready! Press '?' for help."));
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Process FataMorgana protocol
  client.loop();

  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();  // Clear buffer

    switch (cmd) {
      case '0':
        setRotation(0);
        break;
      case '1':
        setRotation(1);
        break;
      case '2':
        setRotation(2);
        break;
      case '3':
        setRotation(3);
        break;

      case 'x':
      case 'X':
        toggleFlipX();
        break;

      case 'y':
      case 'Y':
        toggleFlipY();
        break;

      case 'z':
      case 'Z':
        toggleFlipZ();
        break;

      case 's':
      case 'S':
        cycleSerpentine();
        break;

      case 'r':
      case 'R':
        resetTransforms();
        break;

      case '?':
        printHelp();
        printUseCases();
        printCurrentConfig();
        break;

      default:
        Serial.println(F("Unknown command. Press '?' for help."));
        break;
    }
  }
}
