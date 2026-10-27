(function () {
  const names = window.TABLE.states.map(row => row.name);
  const [first, second] = window.TABLE.signals;
  const signals = {};

  for (const name of window.TABLE.signals) signals[name] = 0;

  window.sense = {
    state: names[names.length - 1],
    signals,
    since: performance.now(),
  };

  addEventListener('keydown', event => {
    if (window.senseLive) return;
    const row = Number(event.key) - 1;
    if (row >= 0 && row < names.length && names[row] !== window.sense.state) {
      window.sense.state = names[row];
      window.sense.since = performance.now();
    }
  });

  addEventListener('pointermove', event => {
    if (window.senseLive) return;
    if (first) signals[first] = event.clientX / innerWidth;
    if (second) signals[second] = event.clientY / innerHeight;
  });
})();