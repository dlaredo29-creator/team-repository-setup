const canvas = document.querySelector('#piece');
const ctx = canvas.getContext('2d');

function resize() {
  canvas.width = innerWidth * devicePixelRatio;
  canvas.height = innerHeight * devicePixelRatio;
}

addEventListener('resize', resize);
resize();

const COLORS = { nobody: '#151b1b', detected: '#17636a', close: '#b95e3d' };

function colorFor(state) {
  if (COLORS[state]) return COLORS[state];
  let hash = 0;
  for (const character of state) hash = (hash * 31 + character.charCodeAt(0)) % 360;
  return `hsl(${hash}, 85%, 50%)`;
}

function drawThumb(centerX, centerY, orientation, unit) {
  const boxes = [
    [10, 10, 15, 15], [5, 5, 8, 10], [4, 1, 7, 6], [9, 8, 6, 5],
    [23, 11, 4, 3], [24, 14, 4, 3], [24, 17, 4, 3], [23, 20, 4, 3],
    [14, 25, 11, 6],
  ];
  const x = centerX - 16 * unit;
  const y = centerY - 16 * unit;
  ctx.fillStyle = '#f2efe9';

  for (const [offsetX, offsetY, width, height] of boxes) {
    let drawX = x + offsetX * unit;
    let drawY = y + offsetY * unit;
    let drawWidth = width * unit;
    let drawHeight = height * unit;

    if (orientation === 'down') {
      drawY = y + (32 - offsetY - height) * unit;
    } else if (orientation === 'sideways') {
      drawX = x + (32 - offsetY - height) * unit;
      drawY = y + offsetX * unit;
      drawWidth = height * unit;
      drawHeight = width * unit;
    }

    ctx.fillRect(drawX, drawY, drawWidth, drawHeight);
  }
}

function frame() {
  const { state, signals, since } = window.sense;
  const held = performance.now() - since;
  const palmWidth = signals.palmWidth || 0;
  const unit = Math.min(canvas.width, canvas.height) / 100;

  ctx.fillStyle = colorFor(state);
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.textAlign = 'center';
  ctx.fillStyle = '#d1d8d2';
  ctx.font = `${Math.max(14, unit * 1.8)}px ui-monospace, Menlo, monospace`;
  ctx.fillText('HAND PROXIMITY', canvas.width / 2, canvas.height * 0.16);
  ctx.fillStyle = '#f2efe9';
  ctx.font = `600 ${Math.max(30, unit * 4)}px system-ui, sans-serif`;
  ctx.fillText(state.toUpperCase(), canvas.width / 2, canvas.height * 0.27);

  const orientation = state === 'close' ? 'up' : state === 'detected' ? 'sideways' : 'down';
  ctx.globalAlpha = Math.min(1, held / 200);
  drawThumb(canvas.width / 2, canvas.height * 0.52, orientation, unit * 1.6);
  ctx.globalAlpha = 1;

  ctx.fillStyle = '#f2efe9';
  ctx.font = `600 ${Math.max(24, unit * 3)}px ui-monospace, Menlo, monospace`;
  ctx.fillText(`PALM WIDTH  ${palmWidth.toFixed(2)}`, canvas.width / 2, canvas.height * 0.82);
  ctx.fillStyle = '#d1d8d2';
  ctx.font = `${Math.max(12, unit * 1.5)}px ui-monospace, Menlo, monospace`;
  ctx.fillText('NORMALIZED IMAGE WIDTH - NOT CM', canvas.width / 2, canvas.height * 0.87);
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);