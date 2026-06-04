#!/usr/bin/env node

/**
 * Test script for FataMorgana discovery protocol
 *
 * Usage:
 *   node test-discovery.js
 */

const http = require('http');

const HOST = process.env.HOST || 'localhost';
const PORT = process.env.HTTP_PORT || 3001;

console.log('\n╔════════════════════════════════════════╗');
console.log('║  FataMorgana Discovery Test Script    ║');
console.log('╚════════════════════════════════════════╝\n');

function httpRequest(method, path, body = null) {
  return new Promise((resolve, reject) => {
    const options = {
      hostname: HOST,
      port: PORT,
      path,
      method,
      headers: {}
    };

    if (body) {
      const payload = JSON.stringify(body);
      options.headers['Content-Type'] = 'application/json';
      options.headers['Content-Length'] = payload.length;
    }

    const req = http.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });
      res.on('end', () => {
        try {
          resolve(JSON.parse(data));
        } catch (e) {
          resolve(data);
        }
      });
    });

    req.on('error', reject);

    if (body) {
      req.write(JSON.stringify(body));
    }

    req.end();
  });
}

async function main() {
  try {
    console.log('Step 1: Getting server configuration...');
    const config = await httpRequest('GET', '/api/config');
    console.log(`  ✓ Server IP: ${config.serverIP}`);
    console.log(`  ✓ Multicast: ${config.multicastAddr}:${config.multicastPort}`);
    console.log(`  ✓ Response Port: ${config.responsePort}\n`);

    console.log('Step 2: Triggering device discovery...');
    const discovery = await httpRequest('POST', '/api/discover');

    if (discovery.ok) {
      console.log(`  ✓ Discovery complete!`);
      console.log(`  ✓ Devices found: ${discovery.devicesFound}\n`);

      if (discovery.devicesFound > 0) {
        console.log('Step 3: Listing discovered devices...');
        const devices = await httpRequest('GET', '/api/devices');

        console.log(`\n╔════════════════════════════════════════════════════════╗`);
        console.log(`║ Discovered Devices: ${devices.deviceCount.toString().padEnd(33)}║`);
        console.log(`╚════════════════════════════════════════════════════════╝\n`);

        devices.devices.forEach((device, index) => {
          console.log(`Device ${index + 1}:`);
          console.log(`  IP:        ${device.ip}`);
          console.log(`  MAC:       ${device.mac}`);
          console.log(`  Hostname:  ${device.hostname}`);
          console.log(`  Firmware:  ${device.firmware}`);
          console.log(`  LEDs:      ${device.ledCount}`);
          console.log(`  Mapping:   ${device.mapping.modeName} (${device.mapping.mode})`);

          if (device.mapping.mode === 0) {
            console.log(`    Row:     ${device.mapping.rowIndex}`);
            console.log(`    Pixels:  ${device.mapping.linePixels}`);
          } else if (device.mapping.mode === 1) {
            console.log(`    Column:  ${device.mapping.columnIndex}`);
            console.log(`    Pixels:  ${device.mapping.linePixels}`);
          } else if (device.mapping.mode === 2) {
            console.log(`    Rect:    (${device.mapping.rectX},${device.mapping.rectY}) ${device.mapping.rectWidth}x${device.mapping.rectHeight}`);
            console.log(`    Serpentine: ${device.mapping.serpentine}`);
          }

          console.log(`  Stats:`);
          console.log(`    Accepted: ${device.lastFrame.acceptedPackets}`);
          console.log(`    Rejected: ${device.lastFrame.rejectedPackets}`);
          console.log(`    Rendered: ${device.lastFrame.renderedFrames}`);
          console.log('');
        });

        console.log('Step 4: Calculating coverage (24x100 image)...');
        const coverage = await httpRequest('GET', '/api/coverage?width=24&height=100');

        console.log(`\n╔════════════════════════════════════════════════════════╗`);
        console.log(`║ Coverage Analysis                                      ║`);
        console.log(`╠════════════════════════════════════════════════════════╣`);
        console.log(`║ Image Size:      ${coverage.imageSize.width}x${coverage.imageSize.height}${' '.repeat(38)}║`);
        console.log(`║ Total Pixels:    ${coverage.totalPixels}${' '.repeat(41)}║`);
        console.log(`║ Covered Pixels:  ${coverage.coveredPixels}${' '.repeat(42)}║`);
        console.log(`║ Coverage:        ${coverage.coveragePercent}%${' '.repeat(37)}║`);
        console.log(`╚════════════════════════════════════════════════════════╝\n`);

        coverage.devices.forEach((device, index) => {
          console.log(`  Device ${index + 1} (${device.ip}):`);
          console.log(`    Mode:   ${device.mode}`);
          console.log(`    Region: (${device.x},${device.y}) ${device.width}x${device.height}`);
          console.log(`    Pixels: ${device.pixels}`);
          console.log('');
        });

        console.log('✅ All tests passed!\n');
        console.log('Next steps:');
        console.log('  • Send a test frame: node test-protocol.js gradient 24 100 0');
        console.log('  • Check ESP web interface: http://' + devices.devices[0].ip);
        console.log('  • Monitor ESP serial output: pio device monitor\n');

      } else {
        console.log('⚠️  No devices found!');
        console.log('\nTroubleshooting:');
        console.log('  1. Check ESP is powered on and connected to WiFi');
        console.log('  2. Verify ESP serial output shows: "Joined multicast group"');
        console.log('  3. Check router allows multicast (IGMP enabled)');
        console.log('  4. Ping ESP IP to verify network connectivity');
        console.log('  5. Try: ping 239.255.42.1 (should work if multicast routing OK)\n');
      }

    } else {
      console.error('✗ Discovery failed:', discovery.error || 'Unknown error');
      process.exit(1);
    }

  } catch (error) {
    console.error('\n✗ Test failed:', error.message);
    console.error('\nIs the server running?');
    console.error(`Try: node server.js\n`);
    process.exit(1);
  }
}

main();
