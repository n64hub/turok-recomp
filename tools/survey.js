// Turok: Dinosaur Hunter's survey for the page's plans (N64Recomp-browser's tools/play_page.mjs and record_shaders.mjs
// load it with an evalFile step): where the game's main loop is, so a plan presses through the menus by what the game
// shows rather than by time. There is no decomp: the address was found in the main loop's code (CLAUDE.md, Facts about
// the game).
//
// The main loop (0x80008604) runs a state machine on the word at 0x80119534: 0 starting, 6 loading, 1 the legal
// screen, 7 between two states, 2 the intro, 3 the title menu, 4 the game (a level's fly-over, then play).
(() => {
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  const names = { 0: 'start', 1: 'legal', 2: 'intro', 3: 'title', 4: 'game', 6: 'load', 7: 'change' };
  const t1 = window.t1 = {
    state: () => n64r(0x80119534),
    where: () => `state ${t1.state()} (${names[t1.state()] ?? '?'})`,
    // Waits up to ms for the main loop to be in the state, true once it is.
    reached: async (state, ms = 60000) => {
      for (const t0 = Date.now(); Date.now() - t0 < ms; await sleep(100)) {
        if (t1.state() === state) return true;
      }
      console.log(`[survey] not in state ${state} after ${ms} ms: ${t1.where()}`);
      return false;
    },
  };
  return true;
})();
