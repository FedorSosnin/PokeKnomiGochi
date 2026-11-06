# How PokeKnomiGochi works

Everything your Pokémon needs, what each action does, and how to fill the Pokédex.

## The symbols

| Symbol | Meaning | What it does |
|---|---|---|
| 🍖 | Food | Drops fastest. Below 30, joy drains faster. |
| 😊 | Joy | Earned mostly by playing and battling. |
| ⚡ | Energy | Needed for battles. Restored by sleep. |
| 🧼 | Hygiene | Restored by a bath. Each poop makes it drop faster. |
| 💞 | Bond | Grows with care, up to 20 points a day. |
| ⚖️ | Weight | Every meal adds a little (berry +4, candy +12), and eating while already full adds 6 more. It burns off slowly (1 every 6 minutes, slower asleep), and games and battles burn more. Above 50, energy drains twice as fast; above 60 it looks chubby. |
| 💪 | Strength | Trained by battling. |
| 💨 | Speed | Trained by playing active games. |
| 🛡️ | Defense | Trained by staying well cared for. |

## Taking care of it

| Action | 🍖 | 😊 | ⚡ | 🧼 | 💞 | ⚖️ | Limit |
|---|---|---|---|---|---|---|---|
| 🤚 **Pet** | | +5 | | | +1 | | Only the first 3 pets each hour add joy |
| 🍓 **Favorite berry** | +35 | +10 | | | +2 | | |
| 🫐 **Other berries** | +25 | | | | | | |
| 🍬 **Candy** | +10 | +12 | Full | | | +12 | 2 a day (3 on the day the visitor brings one): a red number on it shows how many are left, and it leaves the menu at 0 |
| 🛁 **Bath** | | | | Full | +1 | | Also clears every poop |
| 🌙 **Sleep** | | | +6 a minute | | | | Turn the light off, then back on to wake it |

There are three berries, a Razz, an Oran and a Pinap Berry, and each Pokémon has one favorite. Try them to find out which: once found, it shows on the Pokémon's card.

## Games

Games don't raise anything if you quit by swiping up.

| Game | 😊 | ⚡ | 🍖 | 💨 | Bond 💞 | Burns ⚖️ |
|---|---|---|---|---|---|---|
| 🍓 **Berry Drop** | +5, plus 2 per point | −10, more for high scores | **+5** | Yes | +2 | Yes |
| 🏃 **PokeRun** | +5, plus 2 per 4 seconds | −10, more for long runs | −5 | Yes | +2 | Yes |
| 🏄 **PikaSurf** | +5, plus 2 per 4 seconds | −10, more for long runs | −5 | Yes | +2 | Yes |
| 🕳️ **PokeDungeon** | +5, plus 4 per floor | −10, more for deep dives | −5 | Yes | +2 | Yes |
| ❓ **Who's That?** | +3, plus 3 per right answer | | | | +1 | |

Berry Drop, PokeRun, PikaSurf and PokeDungeon each add at most +35 joy per game. Who's That? adds at most +33. Energy and food won't drop below 5 from a game.

## Battles

Your Pokémon needs at least 15 ⚡ to fight.

| Result | 😊 | ⚡ | 🍖 | 💪 | 💞 | ⚖️ |
|---|---|---|---|---|---|---|
| 🏆 **Win** | +10 | −12 | −5 | +6 | +2 | −5 |
| 💫 **Loss** | −5 | Drops to 5 | −5 | | | −5 |
| 🤝 **Win against a buddy** | +15 | | | | +2 | |
| 🤝 **Loss against a buddy** | +8 | | | | +2 | |

**RUN** leaves a wild battle, but 1 time in 20 it fails: the button says **Couldn't get away!** for the rest of that fight, so it's fight or lose. A battle with a buddy can always be left.

### Moves

Two moves, one per button: on the right the strongest of its own type, on the left the strongest of any other type. They come from its species' Red and Blue level-up attacks (moves that do no damage are left out), up to its level, which goes up 1 every hour of age. Training and wins don't teach moves. A new move shows as **Learned …!** at the start of the next wild battle, and only if it's stronger than the one on its button. Evolving switches to the new form's list.

| Charmander's level | Left | Right |
|---|---|---|
| 1 | Scratch (one wide button) | |
| 9 | Scratch | Ember |
| 30 | Slash | Ember |
| 38 | Slash | Flamethrower |

Ditto copies the wild Pokémon's moves. **[Every Pokémon's moves and levels](MOVES.md)** has the whole list for all 151, with each move's power.

### Results

A loss earns nothing: no training and no bond. After a loss, your Pokémon has to sleep before it can fight again. Beat any wild Pokémon, even one you have, and you get 3 Poké Balls to catch it while it runs back and forth trying to get away. It runs at its own speed (its base speed: a Slowpoke takes about 6 s there and back, an Electrode under 3), speeds up and slows down on the way, and the quick ones sometimes turn back early. Where the ball lands, against the rings that run with it, decides how likely it is to hold: in the small green circle 100%, in the bold second ring 75%, in the faint outer ring 25%. If it doesn't hold, the Pokémon breaks out on one of the three shakes and runs away.

### Stars

Every form in the Pokédex (regular and shiny count apart) has 1–3 ★: its genes, kept simple. Genes are each stat's 90–110%.

| ★ | A catch rolls it | A raised Pokémon's genes (their average) |
|---|---|---|
| ★ | 50% | 90–96 |
| ★★ | 35% | 97–103 |
| ★★★ | 15% | 104–110 |

- **Catching:** once the ball holds, a screen of its own shows the Pokémon and its stars, and stays until you tap its ✕. A new form joins with them ("New in your Pokédex!"). If you have that form and the catch has more stars, it replaces your weaker one at once ("New best!"); otherwise yours stays and the catch goes back to the wild.
- **Raising:** hatching or evolving registers the form with your Pokémon's own stars, and never lowers a form you have.
- **Choosing your next Pokémon:** its genes are rolled inside the band of the form you pick, so a 3★ form hatches a 3★ Pokémon. A random egg rolls 90–110.
- **Trading:** the stars go with the Pokémon. If you already have the form you get, you keep the higher. From a buddy on an older version, the stars are rolled.
- **Where:** each Pokémon's Pokédex page, under its sprite (each form its own row), the trade screen and the next-Pokémon choice. In the Pokédex and zone grids, a Pokémon with a 3★ form (regular or shiny) has three small gold stars under it.

## Training

- **Winning a battle** adds attack training (+6 a win).
- **Every 6 hours of good care** adds defense training (+1). Good care means every need stays at 40 or more. Hours asleep count too, and a need can dip under 40 for up to 15 minutes while it's awake (time to notice and feed it) before the 6 hours start over. The count is saved, so a restart or an update doesn't lose it.
- **Mini games** add speed training: Berry Drop, PokeRun, PikaSurf and PokeDungeon add 1 for every 5 points.
- **Weight** isn't training: every berry adds 4 and candy 12 (6 more if it was already 80% full). Games, battles and time burn it off. Over 50, energy drops twice as fast; 20 or less earns the Fit medal.

Each kind of training stops at 100. Swipe up for the card, then left twice: the **Training** page shows each one's bar, with **MAX** when it's full, and what trains it, plus a row for weight. Training belongs to the Pokémon: a new one starts from zero.

## Party Boost

While at least one buddy or the daily visitor is on your screen:

| Boost | |
|---|---|
| ⚡ **Energy drain** | A quarter less: it drops three minutes in four instead of every minute (overweight too) |
| 💪 **ATK, DEF, SPD** | +2 each in wild battles and PokeDungeon (not in battles between buddies) |

It's the same with one friend or five, so extra devices don't stack it. Tap the green **PARTY BOOST** button for the list of who's here. Tap a friend for its stats (level, HP, ATK, DEF, SPD, and a buddy's Pokédex count), then swipe to a buddy's Battle, Trade and Play menu. The ✕ goes back to the list.

The **daily visitor** brings a candy (a third one that day) and stays 60 minutes. It only arrives while yours is awake, but once it's there it stays even if yours goes to sleep, its minutes still running. It steps out while four buddies fill the row and comes back the same day, and its time survives a restart. Leftover minutes end at midnight. Tap it to see its stats.

## Zones

There are two ways to start a battle:

- 🔘 **The physical button, or Wild Encounter (the last circle, after the zones):** a random wild Pokémon from anywhere, met at your Pokémon's home. The Mac, Windows and web versions have no button, so Wild Encounter is the way there (or Space on a keyboard).
- 🗺️ **The map icon on the home screen:** pick a zone and hunt there. Missing Pokémon show as silhouettes. Each Search leans a little toward the common ones you're missing, never toward the rare ones or the legendary.

| Zone | Pokémon | Who lives there | Rare 💎 | Legendary ⭐ |
|---|---|---|---|---|
| 🌲 **Forest** | 41 | Grass, bugs and forest animals | Tangela, Scyther, Pinsir | Mew |
| 🏖️ **Beach** | 30 | Water and Ice | Jynx, Lapras | Articuno |
| 🪨 **Cave** | 31 | Rock, Ground, Fighting and Dragon, plus Zubat and Clefairy | Onix, Hitmonlee, Hitmonchan, Rhyhorn, Magmar, Omanyte, Kabuto, Aerodactyl, Dratini | Moltres |
| 👻 **Mansion** | 21 | Ghost, Psychic, poison gas and sludge, and machines | Grimer, Mr. Mime, Electabuzz, Ditto, Porygon | Mewtwo |
| 🌾 **Field** | 28 | Birds, Normal, Eevee and fire runners | Growlithe, Ponyta, Farfetch'd, Lickitung, Chansey, Kangaskhan, Tauros, Snorlax | Zapdos |

Every zone's Pokémon are **common**, **rare** or **legendary**:

- **Common** ones turn up all the time. Their evolved forms come half as often.
- 💎 **Rare** ones, and what they evolve into (Dratini to Dragonite, Growlithe to Arcanine...), come about a third as often as a common one.
- ⭐ The **legendary** can turn up on any Search, from the very first, but only about 1 Search in 100. Evolved forms only appear once your Pokémon is high enough level. All 151 can be caught this way, and catching all of a zone earns its zone medal.

✨ About 1 wild Pokémon in 100 is shiny. Its shiny form goes into the Pokédex separately from the regular one, so you can have both, and each can be traded on its own.

## What drops over time

| | 🍖 | 😊 | ⚡ | 🧼 | ⚖️ |
|---|---|---|---|---|---|
| ☀️ **Awake, per minute** | −2 | −1 | −1 | −1, plus −4 per poop | −1 every 3 minutes |
| 🌙 **Asleep** | −1 every 2 minutes, stops at 30 | −1 every 2 minutes, stops at 35 | +6 a minute | −1 every 3 minutes, stops at 45 | −1 every 3 minutes |

Joy also drops 2 more per minute while 🍖 is below 30, and 2 more while 🧼 is below 30.

## Time

The game counts its own minutes, and only a clock nobody can set adds time away. Changing a device's clock moves the hours on screen, never your Pokémon's age, and never hands out an extra day.

| What | How |
|---|---|
| 🕰️ **Time away** | Applied when you come back, up to 2 weeks at once. The Knomi measures it by network time over Wi-Fi. Mac, Windows and the web ask a time server when they start, and keep counting with the computer's own clock, which runs on while it sleeps and can't be set. Without Wi-Fi or a server, time away waits until one answers. |
| 📅 **A new day** | Candies, the visitor and the streak move on at midnight, once 12 hours have been played since the last day began. The day never goes back, so a clock that's wrong and then corrected loses nothing and keeps the streak going. |
| 🌍 **Time zone** | The Knomi uses the city you pick (swipe down, tap the city). Mac, Windows and the web use the computer's. |
| ✋ **Setting the time** | Only on a Knomi without Wi-Fi: on Wi-Fi, and on Mac, Windows and the web, the time screen just shows the clock. |

## Growing up

| What | How |
|---|---|
| ⬆️ **Level** | +1 for every hour of age, up to 100. A Pokémon stays at 100 from then on, so it doesn't grow too strong for battles |
| 🛡️ **Defense** | +1 for every 6 hours with all four needs at 40 or above (asleep counts, short dips are forgiven) |
| ⚠️ **Care mistake** | A need at 10 or below counts as a mistake, at most once an hour, and costs 1 💞 |
| 🏃 **Running away** | With all four needs at 0 for an hour, it's ready to leave. One bit of care resets the clock. |
| 🔄 **Evolving** | At its evolution level (one level later for each care mistake), with all four needs at 40 or more, a red **Evolve** button appears. Tap it to evolve, or **Keep** to stay as it is: the button comes back an hour later. Tapping away from the question leaves the button there. Every species' level is in [Moves](MOVES.md), and [Evolution stats](EVOLUTION.md) shows what each evolution adds. |
| 👋 **Goodbye** | A fully evolved Pokémon offers a goodbye from 3 days old. Choose to stay together and it won't ask again; hold your finger on it whenever you're ready. |
| 🥚 **Next Pokémon** | After a goodbye or letting one go, choose a first form from your Pokédex (Pikachu, not Raichu). A shiny one you have is its own square and always hatches shiny ✨; a regular one has a 25% chance |

## Medals

Swipe from right to left on the home screen to see them all, 6 to a page; the ones you have earned are green with a check mark. Tap any of them to see how to earn it. A new medal drops in from the top of the screen on a ribbon; tap it to jump to its page and see it turn green.

| Medal | How to earn it |
|---|---|
| 🥉 **Level 10** | Reach level 10 |
| 🥈 **Level 25** | Reach level 25 |
| 🥇 **Level 50** | Reach level 50 |
| 🍓 **Berry** | Find its favorite berry |
| 📅 **7-day streak** | Care for it 7 days in a row |
| 💞 **Full bond** | Reach 100 💞 |
| 🌟 **Final form** | Reach a fully evolved Pokémon |
| 🏅 **Fit** | At level 5 or higher, keep ⚖️ at 20 or less with no care mistakes |

### Trainer medals

These stay with you across every Pokémon you raise.

| Medal | How to earn it |
|---|---|
| ✨ **Shiny found** | Raise, catch or trade for a shiny Pokémon |
| 📖 **Pokédex 10 / 50 / 100 / 151** | Register that many Pokémon |
| 💯 **Level 100** | Raise a Pokémon to level 100 |
| 🤝 **First buddy** | Have a buddy's Pokémon on your screen |
| 🎉 **Party of 5** | Have 4 buddies at once: 5 Pokémon on screen |
| 🎮 **Playmate** | Play a game with a buddy (Who's That? Showdown or Poké Ball Volley) |

### Zone medals

| Medal | How to earn it |
|---|---|
| 🌲 **Forest** / 🏖️ **Beach** / 🪨 **Cave** / 👻 **Mansion** / 🌾 **Field** | Catch every Pokémon in that zone, legendary included |

### Level 100 medals

For the end game: each Pokémon you raise to level 100 counts once, and these stay with you.

| Medal | How to earn it |
|---|---|
| 🏆 **Level 100 ×3** | Raise 3 Pokémon to level 100 |
| 🏆 **Level 100 ×5** | Raise 5 Pokémon to level 100 |
| 👑 **Level 100 ×10** | Raise 10 Pokémon to level 100 |
