// logger.js
const COLORS = {
  reset: '\x1b[0m',
  dim: '\x1b[2m',
  gray: '\x1b[90m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  magenta: '\x1b[35m',
  cyan: '\x1b[36m',
  white: '\x1b[37m',
};

const LEVELS = {
  DEBUG: { tag: 'DEBUG', color: COLORS.cyan },
  INFO:  { tag: 'INFO ', color: COLORS.green },
  WARN:  { tag: 'WARN ', color: COLORS.yellow },
  ERROR: { tag: 'ERROR', color: COLORS.red },
};

const enabledLevel = (() => {
  const lvl = (process.env.LOG_LEVEL || 'INFO').toUpperCase();
  const order = ['DEBUG','INFO','WARN','ERROR'];
  const idx = order.indexOf(lvl);
  return idx >= 0 ? idx : 1; // default INFO
})();

function ts() {
  const d = new Date();
  // 2026-02-04T22:06:05.123Z
  return d.toISOString();
}

function log(levelKey, scope, msg, meta) {
  const order = ['DEBUG','INFO','WARN','ERROR'];
  if (order.indexOf(levelKey) < enabledLevel) return;

  const level = LEVELS[levelKey];
  const scopeTxt = scope ? `${COLORS.magenta}${scope}${COLORS.reset}` : '';
  const metaTxt = meta ? ` ${COLORS.dim}${JSON.stringify(meta)}${COLORS.reset}` : '';
  // Format: [timestamp] [LEVEL] (scope) message {meta}
  // Example: 2026-02-04T22:06:05.123Z INFO  (DeviceWS) device connected {"id":"AA:BB:..."}
  console.log(
    `${COLORS.gray}${ts()}${COLORS.reset} ${level.color}${level.tag}${COLORS.reset} ` +
    (scope ? `(${scope}) ` : '') +
    `${msg}${metaTxt}`
  );
}

module.exports = {
  debug: (scope, msg, meta) => log('DEBUG', scope, msg, meta),
  info:  (scope, msg, meta) => log('INFO',  scope, msg, meta),
  warn:  (scope, msg, meta) => log('WARN',  scope, msg, meta),
  error: (scope, msg, meta) => log('ERROR', scope, msg, meta),
};