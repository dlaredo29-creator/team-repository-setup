const LIBRARY = 'https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@1.1.1/vision_bundle.mjs';
const WASM = 'https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@1.1.1/wasm';
const MODEL = 'https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task';

const video = document.querySelector('#camera');
const button = document.querySelector('#start');
const readout = document.querySelector('#readout');
const scores = document.querySelector('#scores');

let landmarker = null;
let lastVideoTime = -1;
let latestLandmarks = [];
let failed = false;

const SIGNALS = {
  present: result => result.landmarks.length > 0 ? 1 : 0,
  palmWidth: result => {
    const hand = result.landmarks[0];
    if (!hand) return 0;
    const indexBase = hand[5];
    const pinkyBase = hand[17];
    return clamp(Math.hypot(indexBase.x - pinkyBase.x, indexBase.y - pinkyBase.y));
  },
};

function clamp(value) {
  return Math.min(1, Math.max(0, value));
}

const SMOOTHING = 0.25;
const signals = window.sense.signals;
let candidate = null;
let candidateSince = 0;

function applyTable(now) {
  const winner = window.TABLE.states.find(row => row.when(signals));
  if (!winner || winner.name === window.sense.state) {
    candidate = null;
    return;
  }
  if (winner.name !== candidate) {
    candidate = winner.name;
    candidateSince = now;
  }
  if (now - candidateSince >= winner.hold * 1000) {
    window.sense.state = winner.name;
    window.sense.since = now;
    candidate = null;
  }
}

function tick() {
  const now = performance.now();
  if (video.currentTime !== lastVideoTime) {
    lastVideoTime = video.currentTime;
    const result = landmarker.detectForVideo(video, now);
    for (const name of window.TABLE.signals) {
      const read = SIGNALS[name];
      if (read) signals[name] += (clamp(read(result)) - signals[name]) * SMOOTHING;
    }
    latestLandmarks = result.landmarks[0] || [];
  }
  applyTable(now);
  requestAnimationFrame(tick);
}

async function start() {
  button.disabled = true;
  button.textContent = 'Loading hand tracker…';
  let HandLandmarker;
  let FilesetResolver;

  try {
    ({ HandLandmarker, FilesetResolver } = await import(LIBRARY));
  } catch (error) {
    console.error(error);
    showFailure('Hand tracking could not load. Check the internet connection and try again.');
    return;
  }

  try {
    const vision = await FilesetResolver.forVisionTasks(WASM);
    const options = {
      baseOptions: { modelAssetPath: MODEL, delegate: 'GPU' },
      runningMode: 'VIDEO',
      numHands: 1,
    };
    try {
      landmarker = await HandLandmarker.createFromOptions(vision, options);
    } catch {
      options.baseOptions.delegate = 'CPU';
      landmarker = await HandLandmarker.createFromOptions(vision, options);
    }
    button.textContent = 'Asking for camera access…';
    video.srcObject = await navigator.mediaDevices.getUserMedia({ video: { width: 640, height: 480 } });
    await video.play();
    video.classList.add('shown');
    window.senseLive = true;
    failed = false;
    button.remove();
    requestAnimationFrame(tick);
  } catch (error) {
    console.error(error);
    showFailure(`Camera did not start: ${error.message}`);
  }
}

function showFailure(message) {
  button.disabled = false;
  button.textContent = 'Did not start. Try again';
  failed = true;
  readout.hidden = false;
  readout.textContent = message;
}

button.addEventListener('click', start);

addEventListener('keydown', event => {
  const key = event.key.toLowerCase();
  if (key === 'd') {
    if (failed) {
      failed = false;
      readout.hidden = false;
      return;
    }
    readout.hidden = !readout.hidden;
  }
  if (key === 'b') scores.hidden = !scores.hidden;
  if (key === 'v') video.classList.toggle('shown');
});

function drawReadouts() {
  if (!readout.hidden && !failed) {
    const { state, signals: currentSignals, since } = window.sense;
    const held = ((performance.now() - since) / 1000).toFixed(1);
    const lines = Object.entries(currentSignals).map(([name, value]) => `${name.padEnd(8)} ${value.toFixed(2)}`);
    readout.textContent = `${window.senseLive ? 'camera on' : 'fake: keys 1-3, mouse'}\nstate    ${state}  (${held} s)\n${lines.join('\n')}`;
  }
  if (!scores.hidden) {
    scores.textContent = latestLandmarks.length
      ? latestLandmarks.map((point, index) => `${String(index).padStart(2, '0')}  x ${point.x.toFixed(3)}  y ${point.y.toFixed(3)}`).join('\n')
      : 'no hand in view (start the camera first)';
  }
  requestAnimationFrame(drawReadouts);
}

requestAnimationFrame(drawReadouts);