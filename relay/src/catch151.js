// Catch 151's relay Worker (wrangler-catch151.toml): the same fetch handler as the original, without exporting the
// Lobby class, which lives in pokeknomigochi-relay; this Worker reaches it through its LOBBY binding (script_name)
import relay from './index.js';
export default relay;
