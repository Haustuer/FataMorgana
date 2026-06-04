#!/usr/bin/env node

/**
 * Test script to verify binary discovery response parsing
 * Tests both legacy 68-byte and current 69-byte discovery responses
 */

// Simulate the binary parsing function from server.js
function parseDiscoveryResponse(buffer) {
  const isLegacyResponse = buffer.length === 68;
  const isCurrentResponse = buffer.length === 69;

  if (!isLegacyResponse && !isCurrentResponse) {
    throw new Error(`Invalid discovery response length: ${buffer.length} (expected 68 or 69)`);
  }

  // Check magic bytes "FATA" (0x46415441)
  const magic = buffer.readUInt32BE(0);
  if (magic !== 0x46415441) {
    throw new Error(`Invalid magic bytes: 0x${magic.toString(16)}`);
  }

  const version = buffer.readUInt8(4);
  const type = buffer.readUInt8(5);

  if (type !== 0x01) {
    throw new Error(`Invalid response type: ${type}`);
  }

  // Parse IP address (network byte order - big-endian)
  const ip = `${buffer.readUInt8(8)}.${buffer.readUInt8(9)}.${buffer.readUInt8(10)}.${buffer.readUInt8(11)}`;

  // Parse MAC address
  const mac = Array.from(buffer.subarray(12, 18))
    .map((b) => b.toString(16).padStart(2, '0').toUpperCase())
    .join(':');

  const chipId = buffer.readUInt32LE(18);
  const uptime = buffer.readUInt32LE(22);

  const ledCount = buffer.readUInt16LE(26);
  const ledPin = buffer.readUInt8(28);
  const brightness = buffer.readUInt8(29);
  const firmwareMajor = buffer.readUInt8(30);
  const firmwareMinor = buffer.readUInt8(31);

  const mappingMode = buffer.readUInt8(32);
  const sampleMode = buffer.readUInt8(33);
  const rowColumnIndex = buffer.readUInt16LE(34);
  const linePixels = buffer.readUInt16LE(36);
  const rectX = buffer.readUInt16LE(38);
  const rectY = buffer.readUInt16LE(40);
  const rectWidth = buffer.readUInt16LE(42);
  const rectHeight = buffer.readUInt16LE(44);
  const serpentine = buffer.readUInt8(46);

  // Parse transform byte (position 47)
  const transformByte = buffer.readUInt8(47);
  const rotation = transformByte & 0x03;
  const flipX = (transformByte & 0x04) !== 0;
  const flipY = (transformByte & 0x08) !== 0;
  const flipZ = (transformByte & 0x10) !== 0;

  const acceptedPackets = buffer.readUInt32LE(48);
  const rejectedPackets = buffer.readUInt32LE(52);
  const renderedFrames = buffer.readUInt32LE(56);
  const lastFrameWidth = buffer.readUInt16LE(60);
  const lastFrameHeight = buffer.readUInt16LE(62);

  // Gamma correction (float, little-endian)
  const gamma = buffer.readFloatLE(64);

  // Legacy responses end at gamma; newer responses append out-of-bounds mode.
  const oobMode = isCurrentResponse ? buffer.readUInt8(68) : 0;

  const modeNames = ['row', 'column', 'rectangle'];
  const sampleModeNames = ['pixel', 'interpolated'];
  const serpentineModeNames = ['none', 'horizontal', 'vertical'];
  const oobModeNames = ['black', 'clamp', 'mirror'];

  return {
    protocol: 'FataMorgana',
    version,
    type: 'discovery_response',
    device: {
      ip,
      mac,
      chipId: chipId.toString(16).toUpperCase().padStart(8, '0'),
      hostname: `ESP-${chipId.toString(16).toUpperCase().substring(0, 6)}`,
      firmware: `${firmwareMajor}.${firmwareMinor}`,
      uptime
    },
    hardware: {
      ledCount,
      ledPin,
      brightness,
      ledType: 'WS2812B'
    },
    mapping: {
      mode: mappingMode,
      modeName: modeNames[mappingMode] || 'unknown',
      sampleMode,
      sampleModeName: sampleModeNames[sampleMode] || 'unknown',
      rowIndex: rowColumnIndex,
      columnIndex: rowColumnIndex,
      linePixels,
      rectX,
      rectY,
      rectWidth,
      rectHeight,
      serpentine,
      serpentineModeName: serpentineModeNames[serpentine] || 'unknown',
      rotation,
      flipX,
      flipY,
      flipZ,
      gamma,
      oobMode,
      oobModeName: oobModeNames[oobMode] || 'unknown'
    },
    status: {
      lastFrameCounter: 0,
      lastFrameWidth,
      lastFrameHeight,
      lastFrameRgbType: 0,
      lastFrameMillis: 0,
      acceptedPackets,
      rejectedPackets,
      renderedFrames
    }
  };
}

function createMockResponse({ includeOobMode }) {
  const response = Buffer.alloc(includeOobMode ? 69 : 68);

  // Magic bytes "FATA"
  response[0] = 0x46;
  response[1] = 0x41;
  response[2] = 0x54;
  response[3] = 0x41;

  // Protocol version and type
  response[4] = 1;
  response[5] = 0x01;

  // Device IP: 192.168.1.100 (big-endian)
  response[8] = 192;
  response[9] = 168;
  response[10] = 1;
  response[11] = 100;

  // MAC: AA:BB:CC:DD:EE:FF
  response[12] = 0xAA;
  response[13] = 0xBB;
  response[14] = 0xCC;
  response[15] = 0xDD;
  response[16] = 0xEE;
  response[17] = 0xFF;

  // Chip ID: 0x1234ABCD (little-endian)
  response.writeUInt32LE(0x1234ABCD, 18);

  // Uptime: 3600000 ms (little-endian)
  response.writeUInt32LE(3600000, 22);

  // LED Count: 256 (little-endian)
  response.writeUInt16LE(256, 26);

  // LED Pin / brightness / firmware
  response[28] = 12;
  response[29] = 80;
  response[30] = 1;
  response[31] = 0;

  // Mapping Mode: 2 (rectangle), Sample Mode: 0 (pixel)
  response[32] = 2;
  response[33] = 0;
  response.writeUInt16LE(0, 34);
  response.writeUInt16LE(16, 36);
  response.writeUInt16LE(0, 38);
  response.writeUInt16LE(0, 40);
  response.writeUInt16LE(16, 42);
  response.writeUInt16LE(16, 44);
  response[46] = 1; // horizontal serpentine
  response[47] = 0x05; // rotation 90deg + flipX

  // Statistics
  response.writeUInt32LE(1523, 48);
  response.writeUInt32LE(12, 52);
  response.writeUInt32LE(507, 56);

  // Last frame: 24x100
  response.writeUInt16LE(24, 60);
  response.writeUInt16LE(100, 62);

  // Gamma correction: 2.2 (float, little-endian)
  response.writeFloatLE(2.2, 64);

  if (includeOobMode) {
    response[68] = 2; // mirror
  }

  return response;
}

function printHexDump(buffer) {
  for (let i = 0; i < buffer.length; i += 16) {
    const chunk = buffer.subarray(i, i + 16);
    const hex = Array.from(chunk)
      .map((b) => b.toString(16).padStart(2, '0').toUpperCase())
      .join(' ');
    const ascii = Array.from(chunk)
      .map((b) => (b >= 32 && b < 127 ? String.fromCharCode(b) : '.'))
      .join('');
    console.log(`  ${i.toString(16).padStart(4, '0')}: ${hex.padEnd(48)}  ${ascii}`);
  }
}

function printParsedResponse(parsed) {
  console.log('Device Information:');
  console.log(`  IP:        ${parsed.device.ip}`);
  console.log(`  MAC:       ${parsed.device.mac}`);
  console.log(`  Chip ID:   ${parsed.device.chipId}`);
  console.log(`  Hostname:  ${parsed.device.hostname}`);
  console.log(`  Firmware:  ${parsed.device.firmware}`);
  console.log(`  Uptime:    ${parsed.device.uptime} ms`);

  console.log('\nHardware:');
  console.log(`  LED Count:    ${parsed.hardware.ledCount}`);
  console.log(`  LED Pin:      ${parsed.hardware.ledPin}`);
  console.log(`  Brightness:   ${parsed.hardware.brightness}`);

  console.log('\nMapping Configuration:');
  console.log(`  Mode:         ${parsed.mapping.modeName} (${parsed.mapping.mode})`);
  console.log(`  Sample Mode:  ${parsed.mapping.sampleModeName} (${parsed.mapping.sampleMode})`);
  if (parsed.mapping.mode === 2) {
    console.log(`  Rectangle:    (${parsed.mapping.rectX},${parsed.mapping.rectY}) ${parsed.mapping.rectWidth}x${parsed.mapping.rectHeight}`);
    console.log(`  Serpentine:   ${parsed.mapping.serpentineModeName}`);
    console.log(`  Rotation:     ${parsed.mapping.rotation * 90} deg`);
    console.log(`  Flip X/Y/Z:   ${parsed.mapping.flipX}/${parsed.mapping.flipY}/${parsed.mapping.flipZ}`);
  }
  console.log(`  Gamma:        ${parsed.mapping.gamma.toFixed(2)}`);
  console.log(`  OOB Mode:     ${parsed.mapping.oobModeName} (${parsed.mapping.oobMode})`);

  console.log('\nStatistics:');
  console.log(`  Accepted Packets:  ${parsed.status.acceptedPackets}`);
  console.log(`  Rejected Packets:  ${parsed.status.rejectedPackets}`);
  console.log(`  Rendered Frames:   ${parsed.status.renderedFrames}`);
  console.log(`  Last Frame:        ${parsed.status.lastFrameWidth}x${parsed.status.lastFrameHeight}`);
}

console.log('\n╔════════════════════════════════════════════════════════╗');
console.log('║    Binary Discovery Response Format Test              ║');
console.log('╚════════════════════════════════════════════════════════╝\n');

try {
  const testCases = [
    { label: 'legacy 68-byte response', includeOobMode: false, expectedOobMode: 0 },
    { label: 'current 69-byte response', includeOobMode: true, expectedOobMode: 2 }
  ];

  for (const testCase of testCases) {
    console.log(`Creating mock ${testCase.label}...`);
    const mockPacket = createMockResponse({ includeOobMode: testCase.includeOobMode });

    console.log('\nHex dump:');
    printHexDump(mockPacket);

    console.log('\nParsing response...');
    const parsed = parseDiscoveryResponse(mockPacket);

    if (parsed.mapping.oobMode !== testCase.expectedOobMode) {
      throw new Error(`Unexpected oobMode ${parsed.mapping.oobMode} for ${testCase.label}`);
    }

    console.log('\n✓ Successfully parsed!\n');
    printParsedResponse(parsed);
    console.log('');
  }

  console.log('✅ All tests passed!');
  console.log('\nPacket sizes: 68-byte legacy and 69-byte current (vs ~300 bytes JSON)');
  console.log('Size reduction: 77%+\n');
} catch (error) {
  console.error('\n❌ Test failed:', error.message);
  console.error(error.stack);
  process.exit(1);
}
