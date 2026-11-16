// piece.js: reads window.sense inside its own frame loop and answers.
//
// What is here now is a gauge: a colour per state and a circle that follows one
// signal. A gauge proves the sensing works and nothing more. Replace all of it
// with the piece your State Table describes. Read the object, never the camera.

const canvas = document.querySelector('#piece');
const ctx = canvas.getContext('2d');
let lastFrame = 0;
let lastState = window.sense.state;
let previousState = lastState;
let transitionStarted = performance.now();
let radius = 0;

function resize() {
  const scale = Math.max(1, devicePixelRatio || 1);
  canvas.width = Math.round(innerWidth * scale);
  canvas.height = Math.round(innerHeight * scale);
  ctx.setTransform(scale, 0, 0, scale, 0, 0);
}
addEventListener('resize', resize);
resize();

const COLORS = { nobody: '#0b0b0d', here: '#1d2a3a', open: '#3a1d2a' };

// A state with no colour above still gets a bright one of its own, picked from
// its name, so a new row in table.js is visible the moment it fires.
function colorFor(state) {
  if (COLORS[state]) return COLORS[state];
  let hash = 0;
  for (const ch of state) hash = (hash * 31 + ch.charCodeAt(0)) % 360;
  return `hsl(${hash}, 85%, 50%)`;
}

function frame(now) {
  const { state, signals } = window.sense;
  const elapsed = lastFrame ? Math.min(64, now - lastFrame) : 16;
  lastFrame = now;
  if (state !== lastState) {
    previousState = lastState;
    lastState = state;
    transitionStarted = now;
  }

  const transition = Math.min(1, (now - transitionStarted) / 450);
  const easedTransition = transition * transition * (3 - 2 * transition);
  ctx.fillStyle = colorFor(previousState);
  ctx.fillRect(0, 0, innerWidth, innerHeight);
  ctx.globalAlpha = easedTransition;
  ctx.fillStyle = colorFor(state);
  ctx.fillRect(0, 0, innerWidth, innerHeight);
  ctx.globalAlpha = 1;

  const targetRadius = state === 'nobody'
    ? 0
    : Math.min(innerWidth, innerHeight) * (0.05 + 0.35 * (signals.open || 0));
  radius += (targetRadius - radius) * (1 - Math.exp(-elapsed / 180));

  if (radius > 1) {
    const sway = Math.sin(now / 1300) * Math.min(innerWidth, innerHeight) * 0.008;
    const centerX = innerWidth / 2 + sway;
    const centerY = innerHeight / 2 + Math.cos(now / 1700) * 5;
    const opacity = Math.min(0.9, radius / 28);
    ctx.globalAlpha = opacity;
    ctx.shadowColor = '#f2efe9';
    ctx.shadowBlur = radius * 0.22;
    const glow = ctx.createRadialGradient(centerX, centerY, radius * 0.25,
      centerX, centerY, radius);
    glow.addColorStop(0, '#f2efe9');
    glow.addColorStop(1, 'rgba(242, 239, 233, 0.72)');
    ctx.fillStyle = glow;
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
    ctx.globalAlpha = 1;
  }

  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
