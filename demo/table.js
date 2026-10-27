window.TABLE = {
  signals: ['present', 'palmWidth'],
  states: [
    { name: 'close', when: s => s.present > 0.5 && s.palmWidth >= 0.35, hold: 0.2 },
    { name: 'detected', when: s => s.present > 0.5 && s.palmWidth >= 0.12, hold: 0.2 },
    { name: 'nobody', when: s => s.present < 0.2 || s.palmWidth < 0.12, hold: 0.2 },
  ],
};