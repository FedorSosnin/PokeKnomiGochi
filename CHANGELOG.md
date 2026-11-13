# Changelog

All notable changes to the firmware. Versions match the number shown at the
**About** page (the last page of **Settings** in the hold menu) and `web/knomi/manifest.json`.

Updating from the [web installer](https://fedorsosnin.github.io/PokeKnomiGochi/)
**without** ticking "Erase device" keeps your Pokémon.

## [2.1.2] - 2026-11-12

- **A CRT screen on the Mac, Windows and the web, on by default:** the game as on an old tube TV. Each scanline is a soft beam that swells where the picture is bright, the colours are richer with a warm glow and halo, and the picture bulges as through curved glass into the tube's dark edge, with a reflection and a little grain. Taps and clicks land on what you see through the curve. They now open at Large; **CRT Screen** in the Mac and Windows menu (a **CRT** button under the web game) turns it off.
- **Bigger, sharper windows (Mac and Windows):** the sizes move up one: Small is about the old Medium, Medium the old Large, and a new Large is nearly as big as the web version's. Every size is drawn sharp: the screen is blown up by whole pixels first, and Small and Large land exactly on a Retina screen's pixels (Large on Windows at 100% and 200% scaling too), so the pixel art stays crisp instead of blurring.
- **A drop shadow under the Mac and Windows apps:** a soft shadow under the round window, as if it floats over the desktop. Clicks go through it to whatever is underneath.
- **Your Pokémon's nickname in battles:** the HP panel and lines like "SPLASHY: DRAGON RAGE!" use the name you gave it, in wild and buddy battles, PokeDungeon and Knomi Tower.
- **Travel:** sliding between Knomi Island and Knomi Tower moves the whole map, so the tower no longer shows at the bottom of the island while it slides. Only the ✕ stays put.

## [2.1.1] - 2026-11-12

- **Evolve when you want:** the red **Evolve** button now has a grey **Not now** under it. **Not now** (or **Keep form**) keeps your Pokémon as it is for good: the button no longer comes back an hour later, or after a restart. Whenever you're ready, hold your Pokémon for 3 seconds: while it can evolve, a box saying "Your CATERPIE is trying to evolve!" with an **Evolve** button sits above the release question. Tapping anywhere else while the buttons are up does nothing, so a slip can't decide. Its next form is asked about once again.
- **Party Boost:** a round button with two balloons and **+2**, in the sky to the right of your Pokémon, instead of the green pill that sat on big Pokémon's heads. It's drawn behind every Pokémon, so it never covers one.
- **Clearer need bars:** FOOD, JOY, ENE and HYG are white with a dark outline, so they read on the sky, the sand, every fill colour and the dark places.
- **The Repel:** 10 minutes of mini games earn one, from 20. Time already played carries over.
- **Hall of Fame:** beating the Legend shows a proper painting: a sunset sky with rays over the tower's battlements, a gold frame with the eight floors' crests, your team on a red carpet, confetti and a gleam.
- **Medals earned once:** saves from before 2.1.0 lost their care medals to each new Pokémon. The ones the save can prove (the 7-day streak, and Lv.10/25/50 once a Pokémon reached 100) come back quietly, so they aren't won a second time.
- **Knomi auto-update:** a new version now installs itself from any page left dimmed (not in a game, a battle or a question), from 7 am to 8 pm, or 10 am to 6 pm while your Pokémon sleeps. Before, it waited for the home screen with the Pokémon awake, which could take days. Knomis on 2.1.0 need this one update by hand: **Settings**, then **Check for update**.

## [2.1.0] - 2026-11-11

- **Knomi Tower:** a new islet at the end of Travel's pier, with a tower of eight type floors and the Legend at the top, then Mewtwo. Climb with your Pokémon and two Pokédex friends, HP carrying over from fight to fight. Every fight fills the balls' bars, and beating the Legend earns a Master Ball every time, plus the Tower Master badge and a Hall of Fame the first time. Also: a floor guide, Strong/Weak picks worked out from the fight itself, a tip after a lost climb, friends who cheer (+5% HP per lost climb, up to +15%), and three challenges once it's beaten. Travel gets a second page: the map slides out along the pier to the tower.
- **Both balls' bars after a battle:** at the end of a battle, the Ultra Ball's and the Master Ball's bars come up together in a panel in the middle of the screen, and this battle's step fills in on each, its number turning green. Before, only the Ultra Ball's bar showed, along the bottom. When a battle earns a ball, its bar fills up first, then the ball's moment.
- **Pages slide:** turning a page in a menu, with the side buttons or a swipe, now slides the old page out and the new one in from that side, instead of swapping them at once: the card, Settings, the Pokédex grid and its entries, a zone's grid and entries, the trophy shelves and medal cards, a buddy's pages, the trade grid, the Wi-Fi list and the time zones. The room behind stays still and only the board, panel or grid moves; the side buttons, the page line and the X stay put. On the Knomi the new page is drawn once and the slide just moves the two pictures, so it runs faster than the menus usually redraw.
- **Progress page tidied:** on the card's Progress page, the evolution lines sit closer under the level's bar, without the big gap.
- **Days counted by the calendar:** a goodbye's "Thank you for N days" and the card's age now count the calendar days since your Pokémon came, both ends included. Before, they were its minutes lived divided by a day's, which could come out a day or more short. A Pokémon from before this update gets its first day worked out from its age.
- **Care medals are earned once:** the medals for raising a Pokémon (levels 10, 25 and 50, its favourite berry, a 7-day streak, full bond, its final form and staying fit) are now kept for good, like the trainer's medals, instead of starting over with each new Pokémon and being earned again.
- **Choosing your next Pokémon looks like the Pokédex:** the grid sits where the Pokédex's does, with its page line and side buttons in the same places, and shiny forms move on it as they do in the Pokédex.
- **Magikarp sleeps on the ground:** asleep on its side, it stood on the tip of the fin hanging under it and seemed to hover; it now lies on its body.

## [2.0.7] - 2026-11-11

- **The moon and the sun out of the way:** in a battle the moon (and the sun by day) sits higher and a little left, over the wild Pokémon instead of behind it, where it looked like part of the sprite. Wild Encounter's sun, which hid under the wild Pokémon's name, moved up there too.
- **The battle pads float on earth:** each pad your Pokémon and the wild one stand on now has a chunk of earth under it, a floating island: dirt under the grass, packed sand on the Beach, rock in the Cave and at the Mansion.
- **The pads hover:** both pads bob gently, each in its own time, with the Pokémon riding them, and yours casts a shadow on the ground below, smaller as it rises.

## [2.0.6] - 2026-11-11

- **Your Pokémon faces its foe:** in a battle your Pokémon is seen from behind, facing the wild one up to its right, as in the Game Boy games: standing, attacking and taking a hit. Every Pokémon's sprites now have these back views (a few without them still face you).

## [2.0.5] - 2026-11-10

- **The backpack is in the middle:** the icons along the bottom are now the Game Boy, the map, the **backpack** (in the middle, where your thumb is), the moon and the bath. The backpack is the one you tap most, so it's no longer out at the edge.

## [2.0.4] - 2026-11-10

- **About page:** the last page of **Settings** has the 3D Print SOS logo in pixel art, spinning like a gold coin: it spins when the page opens and whenever you tap it, and a gold gleam crosses it now and then. Under it are the version, "Made by 3DPrintSOS - Fedor", and **GitHub** and **YouTube** buttons. On Mac, Windows and the web they open in your browser; on the Knomi they show a QR code to scan with your phone.
- **Settings tidied:** the first page has the time, then a row each for **Sound** (a switch) and **Language** (its own name; tap for the next), and **Start Over** under them. On the Knomi, your time zone is the button under the time, and Wi-Fi and updates are the next page. The version moved to About.

## [2.0.3] - 2026-11-10

- **Choose a ball, then Throw:** on the catch screen, a tap on a ball chooses it, in a gold frame, and the **Throw** button above the balls throws it. The button is in the ball's colour and says its name (Throw Poké Ball in red, Ultra Ball in yellow, Master Ball in purple), so a Master Ball can't go by accident. Each catch starts on the Poké Ball; with only Poké Balls, the button is ready and a tap on the ball throws too. The hint is up in the sky now.
- **Throw with the keyboard:** on Mac, Windows and the web, the left and right arrows choose a ball, Space or the up arrow throws it, and once it's in the air the left and right arrows steer it.
- **No swiping out of a battle:** a swipe up no longer closes a battle or a catch, so a failed run can't be undone by swiping away.

## [2.0.2] - 2026-11-10

- **Beach Pokémon stand on the sand:** at home on the Beach, the sand now comes up as far as the other homes' grass. Before, a Pokémon stood on the very edge of the sand with the top of its shadow on the sea, so a big one like Blastoise looked like it was hovering when it stepped.
- **Blastoise sleeps on the ground:** asleep, it sits back with a foot stretched toward you, and it used to stand on that foot with its body in the air (over the sea, on the Beach). It sits on its body now.
- **The Repel bar opens the Repel's page:** at the end of a mini game, tap the Repel bar under **Play Again!** to see the Repel's page on your Pokémon's card: what it does, how many you hold and how far along the next one is.
- **Who's That? buttons are all one width:** the three names are as wide as the bottom one, in Who's That? and in Who's That? Showdown.
- **Wild Encounter brings you back to Knomi Island:** when a Wild Encounter ends, whether you run, win, catch it or it gets away, you're back on the island, ready for the next one, the way a zone's Search brings you back to the zone. Before, it went home every time, which made hunting for shinies slow. The physical button's battle (Space on a keyboard) still ends at home.
- **Check for Updates asks again:** on Mac and Windows, **Update to Version…** in the menu now checks GitHub again before offering, so it offers the newest version. Before, it could offer the one it had found earlier, even after a newer one came out. Offline, it still offers the one it found.

## [2.0.1] - 2026-11-10

- **The hold menu is the way around:** no more swipes on the home screen. Hold your Pokémon for the ring of Settings, Medals, Stats and the Pokédex, which leaves room for more there. Swipes still turn pages inside them, and still quit a game.
- **Searching a zone finds what you're missing:** a Search now leans toward the Pokémon you haven't caught there, rare ones too (less often), the way a Wild Encounter does. Before, a missing rare like Jynx at the Beach turned up only about once in 60 Searches; when it's the last one there, now it's about one in 3 or 4.
- **Updates arrive sooner:** the Knomi, Mac and Windows apps look for a new version every hour instead of once a day, and on a Mac or PC the hours it sleeps count, so a version out overnight is there by morning. Installing still waits until nobody is using the game.

## [2.0] - 2026-11-10

A new look for every screen, on the Knomi, Mac, Windows and the web. Your Pokémon, Pokédex and medals carry over.

- **A world that walks with you:** your Pokémon walks toward you while its home slides by, in five hand-drawn zones (Field, Forest, Beach, Cave and Mansion), the ones outdoors at day, dusk and night.
- **Choose your starter:** Bulbasaur, Charmander, Squirtle and Pikachu walk up to meet you. No more eggs: once a Pokémon has been offered its goodbye, you choose your next from any first form in your Pokédex, shinies included, and the old one walks off over the hill.
- **Items:** an **Ultra Ball** every 25 battles fought to the end (it triples your catch chance), a **Master Ball** every 100 (it always catches) and a **Repel** every 20 minutes of mini games (weaker wild Pokémon stay away for 3 minutes). Each has its own page on the card.
- **Catching:** the balls you hold sit in a row; tap one to throw it. With a Master and an Ultra Ball, the Poké Ball sits in the middle. A ball that hits drops to the ground before the count.
- **The backpack:** berries, candy and Repels on a curved band that slides up out of the bag, its tail pointing into it, spaced out so each is easy to tap; a berry drops onto your Pokémon and it hops. It stays open until you close it.
- **Hover:** on a computer and the web, what's under the mouse lifts a little, with its shadow under it. (No more blue outline.)
- **Smooth on big screens:** on a computer and the web, the need bars, their labels and the icons' and backpack's bands have smooth edges.
- **Hold it for a menu:** hold your finger on your Pokémon for a ring of Settings, Medals, Stats and the Pokédex, with big icons. It stays open until you choose or tap away.
- **Nothing closes on its own:** menus and questions you open stay until you answer or close them: the backpack, the hold menu, release, evolve, goodbye, and a finished trade.
- **Menus:** every menu sits in your Pokémon's bedroom; the card's pages share one layout; "Page 2 of 7" in place of dots.
- **The Pokédex:** each entry turns round to show its Pokémon from every side.
- **Knomi Island:** Travel is a round island with the five zones, its sea, pines and Mansion windows moving.
- **The games:** **Play Again!** at every game's end, and new stairs in PokeDungeon.
- **The games, on a Game Boy:** the mini games sit on a Game Boy's green screen as cartridges in the colours the games came in, and the screen opens at once.
- **Bigger home icons:** easier to tap on the Knomi, with the games as a little grey Game Boy, outlined like the backpack's foods, and a little space between each. Your Pokémon and its world sit higher, so the backpack's foods never cover it. Poops land anywhere around your Pokémon.
- **Pokédex: one entry to the next:** arrows on every entry step through the Pokédex, or a zone's list.
- **Crisper on the Knomi:** round buttons with a dark ring, text and keyboard letters in sizes that land on whole pixels, and Pokédex entries in capitals as on the Game Boy.
- **The trophy room:** all 25 medals as things in one room, a gleam sweeping over the ones you've won. Tap a part of the room for a closer look at that group, every trophy with its name under it, and tap a trophy for its card. Each is drawn in the same pixel-art style as the Pokémon and the food: zone badges in a case, Poké, Great, Ultra and Master Ball statues, a golden Pikachu under glass, bronze, silver and gold cups, photos of your buddies, and rosettes marked with what they're for.
- **Who's That? counts down:** ten seconds for each silhouette, alone or in a Showdown; out of time counts as wrong, and the answer shows.
- **Every Pokémon on its feet:** each one stands and walks on the ground instead of floating over its shadow (Venusaur and Ivysaur on their legs, their heads hanging in front, awake or asleep; Diglett and Dugtrio down in their mounds), and big ones like Snorlax, Lapras and Dragonite come out bigger.
- **The hills in order:** the Forest's big pines and the Mansion's fence and graves stand behind the hill your Pokémon walks on, not over it.
- **Evolving:** while your Pokémon evolves it still goes by its own name, and by the new one once it has.
- **And more:** a new evolution scene, counts in the middle of their circles, more lines under your Pokémon's name, a new click sound, **ERASE EVERYTHING** asks twice, and while your Pokémon sleeps only the home screen dims.
- **Buddies on the hill:** buddies and visitors stand on the hill's curve, lower toward the sides of the screen, instead of in the air.
- **More memory for buddies over the internet:** sprites keep their colours and animations in the Knomi's large memory, leaving more of its small fast memory for Wi-Fi and the buddy relay.
- **The save** has a name of its own: an older save is copied over on the first start, and left as it was.

## [1.51] - 2026-11-06

- **Home screen:** your Pokémon's level now sits small above its name, and the name is bolder, like on a Pokédex entry. In Japanese and Korean the name is a size bigger than the level.

## [1.50] - 2026-11-06

- **Auto-update:** a new version can now go in by itself, with nobody having to tap anything. It's on unless you turn it off: on the Knomi with the **Auto-update** switch on the settings page (swipe down, then left), on Mac and Windows with **Update Automatically** in the menu, and on the web with the **Auto-update** button under the game. It only updates while nobody is using it: the home screen showing, nothing being played with a buddy, and no touch for a minute. The Knomi also waits for daytime with your Pokémon awake, so the restart doesn't light up a dark room. Your Pokémon is kept, as with any update. With it off, they ask first, as before.

## [1.49] - 2026-11-06

- **How to play, with a buddy too:** Poké Ball Volley, Who's That? Showdown and PikaSurf open under the same card as the other games. Each of you presses **Ready, GO!** (or swipes the card away), and the game starts when you both have; until then it says it's waiting. A buddy on an older version starts as before.
- **Poké Ball Volley on green:** the court is a deep green now instead of blue, and its tile on Play together shows the same court. The directions written on the court are gone: the card has them.

## [1.48] - 2026-11-06

- **PikaSurf with a buddy:** Play together has PikaSurf now. You both surf the same course at the same time, each on your own screen, with your buddy's Pokémon riding alongside yours. Whoever lasts longer wins. Crash first and it says you lose, then waits for your buddy to finish; when you're both out, it shows both times and goes back to Play together.
- **Play together tiles:** Poké Ball Volley (on a green court), Who's That? Showdown and PikaSurf are app tiles, like the Games menu.
- **Messages come first:** a new medal, the **EVOLVE!** button and "… wants to tell you" now dim the rest of the screen, and a tap can't reach anything behind them. Tap the message to open it, or tap off it to put it away (the medal is kept; evolving is offered again later; the goodbye as "stay together").
- **The visitor:** tap it to see its stats. It no longer leaves when your Pokémon goes to sleep: once it's here, it stays its hour.
- **PikaSurf:** the rocks are a little bigger, so two side by side leave no gap to sit in: the way through is always an open lane.

## [1.47] - 2026-11-06

- **PikaSurf, a new mini game:** your Pokémon surfs down the screen on a board while the water rushes up, and rocks come at you in three lanes. Tilt, drag your Pokémon or use the arrow keys to dodge them. Pikachu surfs alongside on his own board and never hits one, until he's just faster than you and pulls ahead out of sight. It speeds up like PokeRun, and one rock ends the run. It trains speed and keeps a record, like the other games.
- **How to play:** when a mini game opens, a white card drops in over it, the game waiting dimmed behind, with the ways to move and what the game is about. Press the green **Ready, GO!** or swipe the card away, and the game starts. On the Knomi the cards list tilting and touch; on Mac, Windows and the web they add the arrow keys.
- **App tiles:** the games and the Travel zones are now rounded tiles, framed in white and each one's own colour, with a soft shadow. The games sit on a grid like Travel's, two on top and three below, and a long name takes two lines (Poke / Dungeon).
- **Back to the games:** when a game ends, you're back on the Games menu to pick the next one, not on the main screen.
- **After a win,** once attack training is maxed the battle just ends: no more "maxed" line.
- **The rubber duck** in the bath sits right beside your Pokémon, on the side toward the middle of the screen, and moves with it.
- **Medals explain themselves:** tap any medal to see how to earn it, whether you have it yet, and a ✕ to go back.
- **Who's That? has 10 silhouettes** instead of 5, and the same Pokémon never comes up twice in a game. A perfect game gives up to +33 joy.
- **Berry Drop:** Tilt Catch is now called Berry Drop. Besides tilting and the arrow keys, a finger (or the mouse) held on the screen now leads your Pokémon to it.
- **Evolution stats:** a new page, [Evolution stats](docs/EVOLUTION.md), lists every evolution line with each form's HP, ATK, DEF and SPD and what evolving adds, for all 151 Pokémon. The README's How to play section links to it.

## [1.46] - 2026-11-06

- **Time zones on the Knomi:** swipe down and tap the city under **Time zone**. Each city shows its own time, so pick the one that matches your clock; summer time changes by itself. Knomis start on New York, as before.
- **The clock sets itself:** on Wi-Fi the Knomi's time screen just shows the time (**Set by Wi-Fi**). Without Wi-Fi you still set it by hand, and a time earlier than the one shown now means the next morning, so a Knomi unplugged overnight catches up in one go. Mac, Windows and the web follow the computer's clock and time zone, and their time screen only shows it.
- **Changing the clock doesn't pay:** the game counts its own minutes and only takes time away from a clock nobody can set: network time on the Knomi, and on Mac, Windows and the web a time server plus the computer's own sleep-proof clock. Setting the clock forward no longer levels your Pokémon up, and a new day (candies, the visitor, the streak) needs 12 hours of play since the last one began, so setting the time to 11:59 PM doesn't hand out candies. The day never goes back either: when the real time returns, nothing is lost and the streak carries on.
- **The big clock** is centred whatever the hour, and looks the same in Japanese and Korean.
- **New Sleep and Bath icons:** the moon on the home screen now drifts off with a blue zZ instead of sparkles, and bath is a drop of water instead of a bubble. Giving a bath matches: little drops shower down on your Pokémon and splash around it while soap bubbles foam up and a rubber duck sits beside it, before it sparkles clean.

## [1.45] - 2026-11-05

- **A map for battles:** the middle icon on the home screen is now a folded region map instead of a Poké Ball, and the screen it opens is called **Travel**: the zones are places to travel to and hunt.
- **Party Boost:** with company, energy now drains a quarter slower (it was half).
- **The Mac and Windows apps update themselves:** from PokeMacGochi 1.30 and PokeWinGochi 1.13 on, a new version installs from inside the app, like the Knomi's: it downloads, checks the update's signature, and reopens on the new version with your pet as it was. This version still has to be downloaded by hand once.
- **Zones are a real hunt:** each zone's Pokémon are common, rare or legendary. Common ones turn up all the time; rare ones (and what they evolve into) about a third as often; and the legendary can appear on any Search, not just after the rest, but only about once in a hundred. Searches no longer steer you to what you're missing, except a little toward common ones, so filling a zone takes several times longer. [How it works](docs/HOW_IT_WORKS.md#zones) lists each zone's rare ones.

## [1.44] - 2026-11-05

- **Your bag:** the first icon on the home screen is now a green backpack with an orange pocket. It opens your berries and candy, and will hold other items later.
- **Wild Encounter:** Random Battle is now Wild Encounter, the last circle on the Battle screen. Its window shows your Pokémon's home, and the wild Pokémon is met right there, in the same scene as the main screen, by day or night.
- **Games menu:** brighter windows onto each game, ringed in its own colour, with more going on: berries falling, a Who's That? burst, a leap over a rock and a startled Zubat. The title reads Games, and the windows are centred between it and the ✕, as on the Battle screen.
- **Page arrows:** round arrow buttons at the sides of the Pokédex and of each zone's list turn the page, as well as swiping. There's none before the first page or after the last.
- **Screens stay open:** the Games menu, the Battle screen, a zone's list and a wild battle no longer close by themselves when left alone. A battle with a buddy still ends after a minute, since they're waiting.
- **Moves chart:** [Moves](docs/MOVES.md) shows each move's power, so you can see when a Pokémon learns its strongest move before or after evolving.

## [1.43] - 2026-11-05

- **Sharper on the Knomi:** the Knomi shows the game's 466-pixel picture on its 240-pixel screen. Each screen pixel now comes from a fixed pair of picture pixels (they used to drift by one every 16), and the Pokémon, icons and text are drawn on even pixels so they land whole: no more blurred bands. The Pokédex thumbnails are cropped to the Pokémon, so tall ones like Charmeleon are no longer shrunk by dropping rows, and the web version scales the screen smoothly when it's shown smaller than its own size.
- **The Knomi stays dim asleep:** turning the light off could send the screen to full brightness when the backlight chip had lost count of its level (after an update, for example). Every brightness change now starts the chip from a known level.
- **Games menu:** a round window onto each game, as on the Battle screen, with your own Pokémon in it: the woods of Tilt Catch, a Who's That? silhouette, the PokeRun trail and a PokeDungeon room. They follow day and night.
- **Battle screen:** the zones in two rows of three, with Random Battle last as a round button whose window is a pie of all five zones.
- **Pokédex:** the Pokémon a little smaller and crisper; three small gold stars under a Pokémon with a 3 ★ form, instead of the gold outline; and no thin black ring around the edge of the Pokédex or any other menu.
- **Winning a battle** says "Pikachu fainted..." (the wild one's name) instead of "YOU WON!".

## [1.42] - 2026-11-05

- **Every Pokémon runs its own way:** after a battle, the wild Pokémon's run in front of the Poké Ball follows its speed. A Slowpoke ambles there and back in about 6 seconds, a Pikachu in 4½, an Electrode darts across in under 3. Each one also speeds up and slows down a little, never the same twice, and the quick ones sometimes turn back halfway.
- **A bigger Pokédex:** the Pokédex and each zone's list are now 3 × 3 grids with the Pokémon about twice as big. The number caught sits small over the centred title, and "Page 1 of 17" replaces the dots.
- **Shinies move:** a shiny you have moves in the grid with its sparkles, as on its own page.
- **Gold for 3 ★:** a Pokémon with a 3 ★ form, regular or shiny, has a gold outline around it in the grids.
- **Pokédex pages:** the number sits small over the name, which is bigger and bolder. Tap the Pokémon to give it a pat: it hops and a heart pops up, just for fun.

## [1.41] - 2026-11-05

- **Pokédex stars:** every form in the Pokédex has 1–3 ★, its genes kept simple (regular and shiny apart). Every wild win can now be caught, even a Pokémon you have, and after the ball holds, a screen of its own shows the Pokémon and its stars until you tap its ✕: a new form joins, a better one replaces your weaker one ("New best!"), and one that isn't better goes back to the wild. Stars show on each Pokédex page (where the sprites are now bigger), in trades and when you choose your next Pokémon, and a 3★ form hatches a 3★ Pokémon. Your Pokédex is kept as it is: each form you already have gets its stars once, the one you're raising from its own genes.
- **Five icons on the home screen:** berries, a new handheld for the mini games, the Poké Ball for battles, the moon and the bath, evenly spaced along the bottom. The Poké Ball goes straight to the zones on every device, and Back from the zones goes home. The Games menu is just the four mini games, in the same order everywhere: Tilt Catch, Who's That?, PokeRun, PokeDungeon. The bars and the food picker sit a little higher to make room.
- **Medal banners show up again:** a medal or streak earned away from the home screen (in a battle, a mini game, the Pokédex) used to time out unseen. The banner now waits and slides down when you're back home.
- **Mansion:** the little ghost at the right of its picture is gone.
- **Screenshots:** every one in the docs is now cropped round, like the screen.

## [1.40] - 2026-11-04

- **Random Battle:** a teal button under the zones in Games > Battle meets a wild Pokémon from anywhere, like the Knomi's physical button. The Mac, Windows and web versions have no button, so this is their way to a random battle (Space on a keyboard still works too).
- **PokeRun's obstacles as pixel art:** rocks, bushes, logs, stumps and mushrooms are now drawn like the Pokémon and the home icons, with a thin black outline, and each one's hit box is the size of its art.
- **Tilt Catch in the woods:** two tall trees frame the screen with berries growing in their leaves and vines hanging down, a treeline behind, and darker colors at night.
- **Volleyball court:** Poké Ball Volley is played on a court seen from above, with side lines, an attack line and the net along the top where the ball crosses to your buddy. Your Pokémon has a shadow on the court, and the Poké Ball spins slowly as it flies.

## [1.39] - 2026-11-04

- **Your next Pokémon is a first form:** after a goodbye or letting one go, the Pokédex grid you choose from shows only first forms, since the new one hatches from an egg: Pikachu, not Raichu.
- **Pick a shiny:** a shiny in your Pokédex is its own square with sparkles there, and picking it hatches a shiny for sure. A regular one still has its 1 in 4 chance.
- **Levels stop at 100:** past that a Pokémon would be too strong to battle, so it stays at 100; one that is already older comes back to 100 and stays. The progress page shows a full bar and MAX.
- **Level 100 medals:** a new page of achievements for raising 3, 5 and 10 Pokémon to level 100. Each Pokémon counts once, the one you have now too when it's already there.
- **Poké Ball Volley:** your Pokémon runs left and right as you move it and faces you when it stands still, on the ground instead of just above it.
- **PokeRun:** fallen logs, tree stumps and big mushrooms join the rocks and bushes.
- **PokeDungeon:** the berries are the Razz, Oran and Pinap Berries from the food menu.
- **Keep form, then evolve later:** choosing Keep hides the Evolve button for an hour, then it comes back. Before, it waited for the next level, which never comes at level 100. A new Pokémon also no longer inherits the last one's Keep.
- **Medals arrive in style:** a new medal drops in from the top on a red and blue ribbon, a gold medal with a star over the banner, settles with a little bounce and sparkles. Tap it to open your achievements at its page and see it in green; the tap no longer goes through to your Pokémon. A streak milestone drops in the same way, with a flame.
- **A new photo:** the Knomi in its Poké Ball case running the latest game now opens the README and the PokeWebGochi page's Knomi section.
- **[Moves chart](docs/MOVES.md):** every Pokémon's moves and the level it learns each one, with when it evolves. It's generated from the game's own move data, so it always matches.
- **Moves explained:** the README, How it works and the PokeWebGochi page say how the two battle moves are picked and when new ones are learned.

## [1.38] - 2026-11-04

- **Tilt Catch keeps speeding up:** the berries stopped getting faster after 40 catches, so with arrow keys a game could go on forever. Now they keep speeding up, slowly, with no limit, until your Pokémon can't get under them in time. Your record stays.
- **Real berries in Tilt Catch:** the falling berries are the Razz, Oran and Pinap Berries from the food menu, a random one each time, slowly turning as they fall.
- **Live players:** PokeWebGochi shows how many are playing right now (Knomis, apps and browser tabs together) next to its buttons, and the READMEs show it as a badge.
- **PokeMacGochi runs on older macOS** (PokeMacGochi 1.22): it opens on macOS 12 Monterey and newer, including Sequoia. It only opened on the latest macOS before, because it was built for the Mac that built it.

## [1.37] - 2026-11-04

- **A livelier Poké Ball:** a thrown ball spins all the way to the Pokémon, and during the count to 3 it tips over to one side on each count, as if the Pokémon inside is trying to get out. A red number on the Poké Ball shows how many you have left, like the one on the candy.

## [1.36] - 2026-11-04

- **The wild Pokémon runs for it:** after the battle, while you throw Poké Balls, it runs back and forth across the field in its walking animation, facing the way it goes, with a shadow and a little dust behind it, instead of sliding along the horizon. The rings run with it and the ball lands on the field; how to aim and the odds are the same.
- **Tilt and controllers on Windows** (PokeWinGochi 1.3): a controller's left stick tilts smoothly and walks in PokeDungeon with the D-pad, and a motion sensor that Windows knows about tilts the game like a phone (**Tilt with the Device** in the menu turns it off). On a ROG Ally, mapping the gyro to the left joystick in Armoury Crate makes tilting the Ally tilt the game.
- **Vibration on Windows** (PokeWinGochi 1.2): PokeWinGochi buzzes with the game's sound effects on a controller that can rumble: the built-in one of a handheld PC like the ROG Ally, or an Xbox controller. **Vibration** in its menu turns it off.

## [1.35] - 2026-11-04

- **Back out of a trade any time:** the screen where you pick a Pokémon to give now shows an ✕ (it worked before, but you couldn't see it), and so does waiting for your buddy after you've picked or confirmed. Backing out leaves both Pokédexes as they were.
- **Defense training is fairer:** good care now adds +1 every 6 hours instead of 12, hours asleep count, a need can dip under 40 for up to 15 minutes before the count starts over, and the count is saved, so a restart or an update no longer wipes it.
- **How many are playing:** the buddy relay's [`/stats`](https://pokeknomigochi-relay.fedor-dd0.workers.dev/stats) shows how many Knomis, apps and browser tabs are connected right now, as one number.

## [1.34] - 2026-11-04

- **PokeWinGochi, the game on Windows:** the same round window as the Mac app, for Windows 10 and 11, in one `.exe` with nothing to install. It meets Knomis, Macs and browsers as buddies, lives in the taskbar's notification area, and checks for its own updates.
- **Running can fail:** 1 run in 20 from a wild battle fails. The RUN button turns red and says "Couldn't get away!" for the rest of that fight. A battle with a buddy can always be left.
- **Shinies are rarer:** about 1 wild Pokémon in 100 is shiny now, instead of 1 in 50.

## [1.33] - 2026-11-03

- **Vibration in the browser on Android:** PokeWebGochi buzzes softly with taps, meals, medals, catches and the other sound effects, even with the sound muted. A **Vibration** button under the screen turns it off; it only shows on phones whose browser can vibrate (iPhones can't).
- **No more "Zz" next to the moon** while your Pokémon sleeps: the "Zzz..." under its name already says so.

## [1.32] - 2026-11-03

- **Poké Ball Volley** uses the new Poké Ball from the home screen.
- **The Poké Ball has a round button**, like the real one: a thick black ring around a white center, instead of a square. It's the same ball on the home screen, in catches and in Poké Ball Volley.

## [1.31] - 2026-11-03

- **Wi-Fi updates work with buddies around:** the update check and install failed ("Update failed") while the Knomi was connected to the buddy relay, because two secure connections at once don't fit in its memory. The update now closes the relay first and reconnects it afterwards. Knomis on 1.30 or older need one USB install from the web installer to get this fix.
- **"... wants to tell you" only once:** after you choose to stay together, the goodbye isn't offered again by itself (it used to come back daily and after every restart). A note says how to bring it back: hold your finger on your Pokémon.
- **A bigger ✕:** the close button is a little larger and much easier to hit, especially just beside and below it at the bottom of the round screen.
- **Lag report:** the Knomi logs any pass of its loop longer than 150 ms over USB, with how much went to the network and to drawing, and the slowest in its 5-minute health line.

## [1.30] - 2026-11-03

- **Friends' stats:** on Party Boost, tap a friend to see its stats first: level, HP, ATK, DEF and SPD, and a buddy's Pokédex count. Swipe to its Battle, Trade and Play menu, and the ✕ goes back to Party Boost. The visitor has a stats page too, with its minutes left. Tapping a buddy's Pokémon on the main screen still opens its menu first, with its stats a swipe away. Buddies now send their stats; one on an older version shows "?".
- **Candy refills energy** to full, as well as food and joy.
- **The visitor brings a candy:** a third one on the day it visits ("PIDGEY brought a candy!"), shown on its stats page.
- **Candies left:** a red badge on the food menu's candy says how many are left today.

## [1.29] - 2026-11-03

- **Party Boost:** while a buddy or the daily visitor is around, energy drains half as fast and your Pokémon has +2 ATK, DEF and SPD in wild battles and PokeDungeon. It's the same however many friends are around. The green button at the top now says **PARTY BOOST +2**. Tap it for who's here and what you get, and tap a buddy there for its menu. The Battle Stats page shows the +2.
- **The daily visitor stays an hour,** standing in the row with any buddies. It steps out while yours sleeps and comes back the same day, and remembers its time across restarts. Wild battles are no longer blocked while it visits.

## [1.28] - 2026-11-03

- **Swipes over buddies:** a swipe that starts or ends on a buddy (or its name at the top) stays a swipe, so the card, the Pokédex, the achievements and the clock open as they should. A buddy's name box only shows when your finger rests on it, and a tap still opens its menu.
- **PokeWebGochi:** with the mouse over it, the Poké Ball ring grows to twice its width, like the Mac app's. A change to the page alone now also counts as a new version, so an open tab offers it.

## [1.27] - 2026-11-03

- **Training page:** the card has a new page after the stats, showing ATK, DEF and SPD training (0–100) at full size, with **MAX** when it's full and what trains each one: winning battles trains attack, every 12 hours in a row of good care trains defense, and mini games train speed. Weight has a row too: food adds it, play burns it.
- **Battle Stats:** the stats page is now called Battle Stats.
- **Clearer wins:** a won battle says "ATK TRAINING +6", or "ATK TRAINING MAXED" once there's nothing left to train.
- The README and How it works list what trains each stat.

## [1.26] - 2026-11-03

- **A close button everywhere:** a round gray ✕ at the bottom of every menu and screen closes it: the games menu, the card, the Pokédex, the zones, the clock and settings, the buddy menu, Play together, the egg choice and trades. It replaces "tap: back", "swipe up to go back" and the Back buttons. Swipes still work, and games in play still quit with a swipe up.
- **Achievements get their own screen:** swipe from right to left on the home screen. Every medal, at most 6 to a page in a 2×3 grid so none is cut off, earned ones green with a white check mark.
- **Swipes:** from left to right opens the Pokédex, from right to left the achievements, up the card (now profile, stats and progress) and down the clock.
- **Battle Wins:** the stats page says "Battle Wins", higher up and with a trophy.
- **Caught!** The Poké Ball bursts with light when a catch holds.

## [1.25] - 2026-11-03

- **New look for the home screen:** the four buttons are bigger pixel-art icons without boxes: a Razz Berry, a Poké Ball, a crescent moon and a bubble.
- **New food:** the berries are now a Razz, an Oran and a Pinap Berry, with a new wrapped candy. Each Pokémon still loves the same one as before, and its card now shows that berry's icon once you've found it.
- **Catch rings:** while you aim, rings show around the Pokémon. Land the ball in the small green circle and it always holds, in the bold ring three times in four, in the faint outer ring one time in four. The catch uses the home screen's Poké Ball.
- **Attack effects:** every move shows a little of its type: flames, splashing water, leaves, sparks, ice crystals, bubbles, mind rings, pebbles, gusts or an impact burst.
- **A lost battle earns nothing:** no training and no bond, but your Pokémon is still tired out.
- **Renaming is easy to find:** a pencil sits beside the name on the card, and the name is centered above the Pokémon.
- **Zone Pokédex:** says "Swipe up to go back" at the bottom.
- **Mac:** the ring grows when the mouse is over the window, and only the ring moves it: the whole screen is for touching. Over the screen the mouse is a pointing finger that presses when you click. The app icon is now Gengar.
- **Mac and web:** all four arrow keys walk in PokeDungeon. In the browser, too, the mouse is a finger.

## [1.24] - 2026-11-03

- **Shiny Pokémon in the wild:** about 1 wild battle in 50 brings a shiny, in its official shiny colors with gold sparkles twinkling around it. The battle opens with "Shiny ...!". Beat it and you get Poké Balls even if you already have the regular one.
- **Sparkles:** every shiny twinkles: your own, a buddy's, a visitor, a wild one, and in the Pokédex.
- **Both forms in the Pokédex:** the regular and the shiny form of a species are kept apart. A square with the shiny gets a gold sparkle (and shows the shiny if that's the only one you have), and an entry with both shows them side by side.
- **Trade shinies:** the trade grid shows each form you have as its own square, so a shiny can be traded on its own and the regular one stays. Only the exact form you're raising can't be traded.
- **Zone medals:** a medal for each zone you complete, legendary included: Forest, Beach, Cave, Mansion and Field. They have their own Zones page on the card.
- Every one of the 151 can be caught in the zones; a test now makes sure it stays that way.
- Both players need this version to trade shinies.

## [1.23] - 2026-11-03

- **Poké Ball Volley:** a second game under **Play** with a buddy. Each screen is your half of the court, your Pokémon is the paddle, and a ball over the top of your screen drops in at the top of your buddy's. Tilt or drag to move. Every shot curves, more off the edge of your Pokémon, so standing in the middle doesn't win. The ball speeds up slowly the longer it stays in the air, so a match to 3 points lasts about a minute. Same rewards as Showdown, and it counts for the **Playmate** medal.
- **Who's That? Showdown** now has 10 questions instead of 5.
- **Trades cost a Pokémon:** the one you give is gone from your Pokédex. Your own partner can't be traded, and with only one Pokémon, Trade is greyed out.
- **The catch takes three shakes:** after a hit, the ball rocks three times with a tone and a count to 3. Your Pokémon has a 75% chance to stay in; if it breaks out, it runs away ("... got away!").
- **Buddies while your Pokémon sleeps** stand on both sides again, instead of piling up on the right.
- A buddy's invite to play no longer says it wants to trade.
- Both players need this version for the new games and rules.

## [1.22] - 2026-11-02

- **Play with a buddy:** the buddy menu now has **Play**, next to Battle and Trade, with games only buddies can play together. The first is **Who's That? Showdown**: the same silhouette on both screens at once, the first to tap the right name gets the point, a wrong tap is out for the round, best of 5. Both get joy and bond, the winner a bit more, and the new **Playmate** trainer medal. Both players need this version.

## [1.21] - 2026-11-02

- **Yours stands out:** whenever buddies or a daily visitor are on screen, your Pokémon stands in the middle and a size bigger than everyone else, even next to buddies of the same species. Buddies and visitors stand at the sides.

## [1.20] - 2026-11-02

- **PokeWebGochi:** the game in any browser at https://pokewebgochi.3dprintsos.com, the Knomi screen centered on a black page. It saves in the browser by itself, with **Export save** and **Import save** for backups, plays in one tab at a time, has Small, Medium and Large sizes, and uses a phone's own tilt. It updates itself: a tab left open offers the new version.
- **Buddies through the relay:** browsers can't send the Knomi's local Wi-Fi messages, so a small relay on Cloudflare passes the same messages between devices on the same home network. Knomis and PokeMacGochi join it too, so everything meets everything. It stores nothing, and a device that is alone sends nothing.

## [1.19.1] - 2026-11-02

- The first version delivered over Wi-Fi: no other changes.

## [1.19] - 2026-11-02

- **Buddy parties:** up to 4 buddies (Knomis and PokeMacGochis in any mix) show up at once. With two or more, everyone stands in a row, smaller; tap one to battle or trade with it, and swipe in the buddy menu to switch. New trainer medals: **First buddy** and **Party of 5**.
- **Updates over Wi-Fi (Knomi):** the settings page has a **Check for update** button, and the Knomi also checks once a day while the screen is dimmed. Updates download from the web installer's site into the spare firmware slot, along with the sprites when they changed. The save is never touched, and a new version that fails to start rolls back.
- **Update check (PokeMacGochi):** the Mac app looks for a new version at launch and once a day, tells you once, and has **Check for Updates** in its menu.
- **Public:** the repository and the web installer at https://fedorsosnin.github.io/PokeKnomiGochi/ are now public.
- **Weight matters:** every meal adds a little weight and eating while already full adds more, so overfeeding makes your Pokémon chubby and sluggish. It burns off slowly and faster with games and battles. The Fit medal now needs weight 20 or less.
- **Zone Pokédex:** tap a Pokémon in a battle zone's grid to open its full Pokédex entry.
- **Press and slide:** holding a finger on any button, menu row or key shows it large above your finger, the way the Wi-Fi keyboard does. It only fires when you lift, so if you landed on the wrong one you can slide to the right one first. A quick flick still swipes.
- **Trainer medals:** a new card page with medals that stay with you across every Pokémon you raise: find a shiny, register 10, 50, 100 and all 151 in the Pokédex, and raise a Pokémon to level 100.
- **Stats page:** the card's second page is now called Stats instead of Battle.
- **PokeMacGochi:** the window no longer dims when you leave it alone; only a sleeping pet dims it. Its size and place are kept with your save, and opening it a second time says where the running one is.
- **PokeMacGochi:** the same game as a small round window on a Mac desktop, in a Pokéball ring, with arrow keys for tilt and its own save. Build it with `mac/build_app.sh`; see `host/README.md`.
- **Buddies:** other PokeKnomiGochis on the same Wi-Fi show up on your screen, and you can battle them or trade Pokédex entries. The Knomi stays connected to Wi-Fi to make this work.
- **3D-printed case:** a Poké Ball case for the Knomi, with the print file and recommended colors.
- **Auto-rotating screen (Knomi):** turn it any way you like and the screen, touch and tilt follow. It starts ready for your right hand with the cable going off to the left. It holds still during Tilt Catch and when aiming a Poké Ball.
- **12-hour clock:** the clock now shows the time with AM and PM.
- **Joy is earned:** only the first 3 pets each hour add joy, though it still loves every one. Candy is limited to 2 a day and leaves the menu after that.
- **Tilt Catch feeds your Pokémon:** it now gives +5 food and only costs energy.
- **How it works:** new guide with charts of what everything does.
- **Battle zones (Knomi):** Games > Battle opens five zones to hunt in, with progress and silhouettes of what's missing. Filling a zone brings out its legendary. The Games menu now goes Battle, PokeDungeon, PokeRun, Who's That?, Tilt Catch.
- **PokeDungeon:** explore floor after floor with your own Pokémon. Tap to step or fight, find the stairs, and see how deep you get.
- **Shiny odds:** the next Pokémon has a 25% shiny chance after a goodbye or release.
- **Run-away:** the button now asks first, and offers to take care of the Pokémon instead of letting it go.
- **Next Pokémon:** after a goodbye or release, choose your next Pokémon from your Pokédex. A run-away still gives a random common egg.
- **Moves:** every Pokémon fights with its own level-up moves from Red and Blue, instead of one move per type.
- **Catch results:** each throw ends in a large banner, and a missed ball visibly bounces away.
- **Catching:** winning no longer registers a species; lean to steer a thrown Poké Ball at it.
- **Games menu:** Tilt Catch (replaces the ball game), Who's That Pokémon? and PokeRun. Swipe up from the lower half of the screen to quit any game with nothing earned.
- **Knomi web installer:** `web/knomi/` flashes the firmware and sprites from the browser.
- **PokeRun:** tap to jump rocks and bushes in a forest that keeps getting faster; your best time is saved. The tuning lives in `pokerun.h`, with tests.
- **Daily visitor:** a Pokémon drops by to hang out, then says goodbye.
- **Pokédex info:** tap a registered Pokémon for its type, evolution and a fun fact (English only).
- **Knomi settings page:** swipe left on the clock screen; pick a network and type its password on the device.
- **Wild battles replace the training bag:** turn-based, with the Gen-1 damage formula and type
  chart (one type per species). TACKLE from the start, a type move at level 10. Winning trains
  STRENGTH and registers the rival in the Pokédex. The rules live in `battle.h`, with tests.
- **BTT Knomi V2 support** (`BOARD_KNOMI_V2`): 240x240 panel, sprites in flash, NTP clock, BOOT button (tap: battle, hold 1-3 s: screen, hold 10 s: erase the game, after confirming).

## [1.18] - 2026-10-27

Three contributions from **ViezeVingertjes** (PRs #25, #26 and #27), merged together.

### Added

- **Battery percentage from a LiPo discharge curve** (`batcurve.h`). On battery
  the figure comes from a typical single-cell curve over a smoothed voltage, and
  only goes down while discharging, so the charge-inflated voltage right after
  unplugging does not raise it. On USB or while charging the PMU's own gauge is
  still used.
- **Safe power-off.** When the smoothed voltage stays under 3.30 V for 30 s (and
  not in the first 30 s after boot) the pet is saved and the PMU powers off. The
  PMU is also set to cut at 3.1 V as a backstop. Assumes a 4.2 V LiPo/Li-ion
  cell, like the one the board is designed for.
- **Power policy** (`power.h`): the frame rate follows activity (10 fps in use,
  down to 1 fps when the screen is almost off, 2 fps while the pet sleeps
  untouched), static screens (keyboard, clock, card pages 2-4, starter choice)
  are painted once, the speaker amplifier is off while the pet sleeps or the
  screen is dimmed, and under 20 % battery the brightness is capped and frames
  are slower. About 68 % fewer frames reach the panel over 10 idle minutes.
- **Light sleep** with the screen off or almost off, on battery only. A touch
  wakes it. Never with USB connected, so the serial port for the SD card stays
  available.

### Changed

- Sprites are drawn as runs of equal pixels instead of one rectangle per pixel
  (same output, far fewer draw calls), and `loop()` yields 1 ms on passes that
  do not draw.
- The pet walks by real time, so a lower frame rate does not slow it down.
- Battery is read over I2C every 10 s instead of 2 s when the screen is off.

## [1.17] - 2026-09-18

### Added

- **Korean, the eighth language.** Full UI (85 strings), the three medal name
  lengths and the 151 gen-1 species names, on the same `STRINGS[LANG][StrId]`
  structure as every other language. The text measurement centralised in 1.15
  carries almost all of it; the four byte-based spots a UTF-8 row turned up are
  listed under Fixed.
- `u8g2_font_unifont_t_korean2` for Hangul, selected per language in
  `applyLangFont()` rather than through a single `CJK_FONT`. The Korean added
  uses 296 distinct syllables, all of them in KS X 1001, the 2350-syllable set
  that matches the size of `korean2`; `korean1` holds 478. Coverage was checked
  as membership in KS X 1001 rather than by walking the font table, and the
  ASCII in the strings is assumed present. Not yet checked on the board:
  whether `CJK_SIZE_DIV` suits `korean2` as it does the Japanese subset.

### Fixed

- **The release dialog could cut a Korean name mid-character.**
  `renderRelease()` built into `char q[28]`. `"%s 놓아줄까요?"` is 17 bytes
  before the name, so names of four syllables or more (34 of the 151) did not
  fit, and `snprintf` truncated inside a 3-byte sequence, leaving a tail the
  font cannot decode. Now 48; the longest case needs 33 bytes including the
  terminator.
- **Three places sized and positioned text with `strlen()`**, which counts
  bytes: the stat-card header, the species name under a nickname, and the
  gallery detail header. On a UTF-8 row the centring offset and the auto-shrink
  threshold worked off a byte count rather than a width. All three now go
  through `textW()` / `centerX()`. With the classic font `textW()` returns
  `strlen()*6*size`, the same expression as before, so the six Latin languages
  land on the same coordinate at the same size, checked at every length from 1
  to 25.
- **Three Japanese texts were cut short**, all already in 1.15 and 1.16.
  Their buffers were sized when every string was one byte per character, and
  `snprintf` truncates without a trace. The minigame record read `きろく 2`
  with a best of 219, since `char rec[12]` left room for one digit. The streak
  milestone banner never fit at all: `"%u にちれんぞく！"` passes 20 bytes with a
  single digit, and the cut landed mid-character. The streak line on the stat
  card lost digits from 100 days on. The release dialog fixed above was the
  fourth.
- **Every formatted text is now checked against its buffer, in all eight
  languages.** A new test in `test/test_tools.py` finds each
  `snprintf(buf, sizeof(buf), T(...))` in the sketch, reads the size of `buf`,
  and computes the worst case per language from the argument types (5 digits
  for `%u`/`%d`, 10 for `%lu`, and a declared maximum for each `%s`). It is what
  caught the three above, and it would have caught all four. To keep it passing
  by type rather than by typical value, four more buffers get headroom they had
  not yet needed: the medal count, the next-level line, the evolution countdown
  and the profile line.
- **`tools/test_i18n_formats.py` reads only the first `len(LANGS)` language
  blocks**, and `LANGS` listed six, so later rows were skipped without a
  warning. It now lists all eight, and the docstring notes that it needs to
  stay complete.
- **`test/test_i18n.cpp`'s `LANG_NAME[LANG_COUNT]` had six entries**, fewer
  than `LANG_COUNT`, so every entry past the sixth was null. Four of the tests
  build a label from `LANG_NAME[lang]` on every iteration, not only when a
  check fails, so each run passed a null pointer to `%s`. Now lists all eight.

## [1.16] - 2026-09-09

### Fixed

- **Japanese was silently dropping every `！` and `？`.** 21 exclamation marks
  and 4 question marks — a quarter of the Japanese strings, including the mood
  line on the main screen (`ごきげん！` rendered as `ごきげん`). The cause is that
  `u8g2_font_unifont_t_japanese1` carries kana and kanji but no CJK punctuation
  at all — no `！？。、「」` — and `Arduino_GFX` draws nothing for a glyph it can't
  find, without even advancing the cursor, so the loss is invisible in the
  source and on screen. Switched to `japanese3`, the only U8g2 Japanese font
  that has them. Same unifont family: the metrics header is byte-identical and
  all 226 codepoints the firmware uses have byte-identical bitmaps, so nothing
  moves. Costs 102 KB of flash (705 KB → 810 KB, 22% → 25% of the app
  partition); RAM is unchanged.
- **Nidoran♀ and Nidoran♂ both showed as ニドラン in Japanese**, leaving #29
  and #32 indistinguishable in the Pokédex. No U8g2 unifont subset carries
  ♀ (U+2640) or ♂ (U+2642) — not `japanese*`, `korean*` or `chinese*` — and
  `Arduino_GFX` draws nothing for a glyph it can't find, without even
  advancing the cursor. `gen_names.py` already substituted F and M on the
  Latin path; it now does the same on the UTF-8 one, so the names read
  ニドランF and ニドランM. Convention chosen by @usakomint, who preferred it
  over ニドラン(女)/(男) as closer to a species name and shorter on a small
  round screen.

Both confirmed on hardware by @usakomint, who also — without knowing it —
supplied the evidence for the first one: the missing `！` was visible in a
photo he posted of the v1.15 main screen.

## [1.15] - 2026-09-08

### Added

- **Japanese.** UI, medals and all 151 species names in katakana (フシギダネ,
  ピカチュウ). Selectable from the settings screen like any other language.

### Changed

- Text measurement and cursor positioning are now centralized (`textW()`,
  `centerX()`, `setCur()`, `setSize()`, `printT()`), replacing ~180 sites that
  assumed one byte per character and a fixed 6 px width. Latin languages render
  identically — the equivalence is arithmetic, not approximate. This is what
  makes CJK possible without a per-language branch at every draw site, and
  Chinese and Korean now only need their strings.
- Bar positions on the stat card and the care row are derived from the width of
  the widest translated label instead of fixed coordinates, so they stay aligned
  in any language. With Latin labels the numbers come out identical to before.
- New dependency: **U8g2**, for its CJK font data only.

Japanese font handling, readability and layout were all tuned against real
hardware by @usakomint, who found and diagnosed every issue in the process.

## [1.11] - 2026-09-07

### Fixed

- **The whole loop froze for exactly 1 second when touching the screen.**
  `touch.getPoint()` hung on a sleeping CST9217 for the ESP32 I2C driver's
  default 1000 ms timeout, which `Wire.setTimeOut(50)` doesn't bound because
  SensorLib doesn't route through it. Most visible in the ball minigame, where
  taps come fast and two or three stalls ran together — reported as "freezes for
  2–3 seconds on ball tap". An address-only Wire transaction before the read
  fixes it. Measured on hardware over equivalent play (~470 touch reads): 5
  stalls in 60 s before, 0 after. Reported by @gingsiro.

## [1.10] - 2026-09-07

### Fixed

- TPK2 sprites with zero-length frame durations spun `pmdFrameAt()` forever,
  hanging the render loop. These files arrive over USB, so they aren't trusted
  input: durations are clamped at load time and the loop is bounded regardless.
- `thumbs.bin` was mapped into PSRAM without bounding its size or checking that
  the offsets it contains point inside what was actually read. A truncated file
  (plausible — the transfer takes minutes) could read past the allocation.
- The serial `PUT` handler ignored `f.write()`'s return value, so a full SD card
  still reported `DONE` while leaving a truncated file behind. It also took the
  destination path straight from the serial line, so a crafted `PUT` could write
  anywhere on the card; writes are now confined to `/mons/`.
- Touches just outside the Pokédex grid and the on-screen keyboard were treated
  as cell 0. The guard divided first and then checked for a negative index, but
  integer division truncates toward zero, so it never saw one. On a round screen
  that strip is exactly where a finger lands near the edge.

All four from @ajtudela.

## [1.9] - 2026-09-07

### Fixed

- All ~13 game timers compared `millis()` against an absolute deadline, which
  gives the wrong answer once `millis()` wraps at ~49.7 days of uptime: a timer
  could stay active forever, or a ceremony cut off instantly.
- The sprite transfer restarted from scratch on every attempt. It now records
  each confirmed file, so a reload after a dropped connection doesn't redo the
  full ~40 MB / ~10 min.
- Minigame physics advanced per frame rather than per unit of time, so the ball
  fell measurably slower with a large sprite on screen (Charizard) than a small
  one (Diglett) — high scores weren't comparable between species.

### Security

- `esp-web-tools` was loaded from a floating `@10` range with no integrity
  check. Pinned to 10.4.0 with a verified sha384 SRI hash.

All from @ajtudela.

## [1.8] - 2026-09-07

### Fixed

- **The level counter wrapped to 0 after ~10.6 days of play.** `level()`
  returned `uint8_t` and, at 60 minutes per level, the level is the hour count —
  so it rolled over at 256. Reachable in ordinary play, since "stay together"
  postpones the farewell indefinitely. Now `uint16_t`, capped at 999.
- Two related truncations found while fixing it: `calcStat()` took the level as
  `uint8_t`, so stats would have overflowed anyway; and `canEvolveNow()` compared
  against `(uint8_t)(evolveLevel + careMistakes)`, which wrapped with many care
  mistakes and let evolution happen early. The stat card already computed that
  threshold as `int`, so the screen and the actual check could disagree.

Diagnosed by @ajtudela.

## [1.7] - 2026-09-07

### Fixed

- **The UI had no accents, and the assumption behind that was wrong.** The code
  stated in two places that the font had none. In fact `glcdfont.h` is a full
  256-glyph CP437 table and `write()` doesn't filter bytes ≥ 0x80. Spanish now
  has its tildes and ñ, German its umlauts, French its accents. Characters are
  written as single bytes in octal, never UTF-8, because the UI centres with
  `strlen(t) * 6`. Only strings whose length was unchanged were touched.
  Uppercase accented vowels other than É aren't in CP437, so `EVOLUCION` and
  `PROGRES` stay unaccented, as do Portuguese ã/õ.
- German grammar, from the same contributor: "Wähle deinen Starter" (accusative),
  "Leb wohl" as two words, "Entwicklung" rather than "Entwickelt", and more.

Discovered by @danielberndt.

## [1.6] - 2026-09-07

### Fixed

- **Short sound effects were inaudible.** The amplifier was switched on 8 ms
  before each effect, far less than the NS4150B needs to settle, so a 35 ms tap
  finished before the speaker woke up. The boot jingle survived because it's
  ~440 ms — which is why sound seemed to work at startup and nowhere else. The
  amplifier now stays powered while sound is on and the pet is awake, matching
  Waveshare's reference. Notes and amplitude are unchanged: this doesn't alter
  the sound design, it just makes it audible.

Diagnosed by @djyf1.

## [1.5] - 2026-08-19

### Added

- Localized Pokémon names in French and German (Bulbizarre, Bisasam…). Only
  those two differ in gen 1; Spanish, Italian and Portuguese officially use the
  English names. Names come from PokéAPI via `tools/gen_names.py`.

## [1.4] - 2026-08-07

### Fixed

- **Bond was mathematically stuck.** It could gain at most 12 points a day, but
  a single care mistake cost 3 and could repeat every 30 minutes — up to 144 a
  day. With poops draining hygiene, mistakes chained and bond never recovered.
  Two players independently reported it pinned at exactly 10, which is the
  first-day ceiling of 13 minus one mistake. Daily cap 8 → 20, penalty 3 → 1,
  cooldown 30 → 60 min.

## [1.3] - 2026-08-07

### Fixed

- **The egg hatched by itself during starter selection.** Game time kept running
  while the starter screen was open, so taking more than 3 minutes to choose
  meant the egg hatched into the species already rolled — no egg animation, and
  the player's choice ignored. Three community reports matched (Pikachu twice,
  Eevee once). Time now freezes until a starter is chosen.

## [1.2] - 2026-06

### Added

- Firmware version shown at the bottom of the clock/settings screen.
