#!/usr/bin/env python3
"""Tests of the workshop tools (tools/) and of the generated files.

  python3 test/test_tools.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, 'tools')
sys.path.insert(0, TOOLS)


class TestToolsCompile(unittest.TestCase):
    def test_all_scripts_compile(self):
        scripts = sorted(f for f in os.listdir(TOOLS) if f.endswith('.py'))
        self.assertTrue(scripts, 'no script found in tools/')
        for name in scripts:
            with self.subTest(script=name):
                with open(os.path.join(TOOLS, name), encoding='utf-8') as fh:
                    src = fh.read()
                try:
                    compile(src, name, 'exec')
                except SyntaxError as e:
                    self.fail(f'{name} does not compile: {e}')

    def test_executable_scripts_declare_a_shebang(self):
        for name in sorted(f for f in os.listdir(TOOLS) if f.endswith('.py')):
            path = os.path.join(TOOLS, name)
            if not os.access(path, os.X_OK):
                continue
            with open(path) as fh:
                self.assertTrue(fh.readline().startswith('#!'),
                                f'{name} is executable but has no shebang')


class TestTimeZones(unittest.TestCase):
    """The Knomi's time zones (timezones.h): every POSIX rule gives the same UTC offset as the real zone
    (the system's tz database), hour by hour for three years, summer time included."""

    @classmethod
    def setUpClass(cls):
        with open(os.path.join(ROOT, 'timezones.h'), encoding='utf-8') as fh:
            src = fh.read()
        cls.rows = re.findall(r'\{ "([^"]+)", "([^"]+)", "([^"]+)" \}', src)
        cls.default = int(re.search(r'#define TIME_ZONE_DEFAULT (\d+)', src).group(1))

    def test_the_list(self):
        self.assertGreaterEqual(len(self.rows), 40)
        self.assertEqual(self.rows[self.default][0], 'New York')  # every Knomi before the picker
        cities = [r[0] for r in self.rows]
        self.assertEqual(len(cities), len(set(cities)), 'a city is saved by its name')
        for city, _, posix in self.rows:
            self.assertLess(len(city), 24, city)  # the saved name's buffer
            self.assertNotIn('<', posix, f'{city}: plain abbreviations only')

    def test_rules_match_the_real_zones(self):
        try:
            from zoneinfo import ZoneInfo
            import time
            time.tzset
        except (ImportError, AttributeError):
            self.skipTest('needs zoneinfo and time.tzset')
        from datetime import datetime, timezone
        import time
        start = int(datetime(2027, 1, 1, tzinfo=timezone.utc).timestamp())
        end = int(datetime(2030, 1, 1, tzinfo=timezone.utc).timestamp())
        old = os.environ.get('TZ')
        try:
            for city, iana, posix in self.rows:
                try:
                    zone = ZoneInfo(iana)
                except Exception:
                    self.skipTest(f'no tz database entry for {iana}')
                os.environ['TZ'] = posix
                time.tzset()
                for t in range(start, end, 3600):
                    rule = time.localtime(t).tm_gmtoff
                    real = int(datetime.fromtimestamp(t, zone).utcoffset().total_seconds())
                    if rule != real:
                        self.fail(f'{city}: {posix} gives {rule}s at '
                                  f'{datetime.fromtimestamp(t, timezone.utc)} UTC, {iana} {real}s')
        finally:
            if old is None:
                os.environ.pop('TZ', None)
            else:
                os.environ['TZ'] = old
            time.tzset()


class TestDexData(unittest.TestCase):
    """Invariants of the Pokedex data source."""

    @classmethod
    def setUpClass(cls):
        from dex_data import DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY, SLUGS
        from dex_stats import BASE_STATS
        from dex_names import LOCAL_NAMES
        cls.DEX, cls.ACCENTS, cls.CLASSIC = DEX, TYPE_ACCENTS, CLASSIC
        cls.RARE, cls.LEGENDARY, cls.SLUGS = RARE, LEGENDARY, SLUGS
        cls.STATS, cls.NAMES = BASE_STATS, LOCAL_NAMES
        cls.byNum = {row[0]: row for row in DEX}

    def test_all_151_species_present_without_gaps_or_duplicates(self):
        nums = [row[0] for row in self.DEX]
        self.assertEqual(len(nums), 151)
        self.assertEqual(sorted(nums), list(range(1, 152)))

    def test_slugs_are_unique_and_lowercase(self):
        slugs = [row[1] for row in self.DEX]
        self.assertEqual(len(set(slugs)), len(slugs), 'duplicate slugs')
        for s in slugs:
            self.assertEqual(s, s.lower(), f'slug with uppercase: {s}')
            self.assertTrue(s.isascii(), f'non-ASCII slug: {s}')

    def test_display_names_are_uppercase_ascii(self):
        # the firmware's GFX font has no accents or lowercase letters
        for num, slug, display, *_ in self.DEX:
            self.assertTrue(display.isascii(), f'{num} {display} is not ASCII')
            self.assertEqual(display, display.upper(), f'{num} {display} has lowercase letters')
            self.assertLessEqual(len(display), 12, f'{num} {display} does not fit on screen')

    def test_evolutions_point_to_real_species(self):
        for num, slug, display, kind, evo, lvl in self.DEX:
            if evo:
                self.assertIn(evo, self.byNum, f'{display} evolves into {evo}, which does not exist')
                self.assertNotEqual(evo, num, f'{display} evolves into itself')
                self.assertGreater(lvl, 0, f'{display} evolves without a level')
            else:
                self.assertEqual(lvl, 0, f'{display} is a final form but has level {lvl}')

    def test_no_evolution_chain_loops(self):
        for num in self.byNum:
            seen, cur = set(), num
            while cur and self.byNum[cur][4]:
                self.assertNotIn(cur, seen, f'evolution loop from {num}')
                seen.add(cur)
                cur = self.byNum[cur][4]
                self.assertLessEqual(len(seen), 5, f'chain too long from {num}')

    def test_all_types_have_a_color(self):
        for num, slug, display, kind, *_ in self.DEX:
            self.assertIn(kind, self.ACCENTS, f'{display}: type "{kind}" has no color')

    def test_colors_are_6_digit_hex(self):
        for kind, col in self.ACCENTS.items():
            self.assertRegex(col, r'^#[0-9a-fA-F]{6}$', f'odd color for {kind}: {col}')

    def test_rarities_point_to_base_forms(self):
        evolved = {row[4] for row in self.DEX if row[4]}
        for num in sorted(self.RARE | self.LEGENDARY):
            self.assertIn(num, self.byNum, f'rarity for a nonexistent species: {num}')
            self.assertNotIn(num, evolved,
                             f'{self.byNum[num][2]} only comes from evolution: rarity is useless for it')
        self.assertFalse(self.RARE & self.LEGENDARY, 'a species cannot be both rare and legendary')

    def test_classic_starters_exist(self):
        evolved = {row[4] for row in self.DEX if row[4]}
        for num in self.CLASSIC:
            self.assertIn(num, self.byNum, f'nonexistent starter: {num}')
            self.assertNotIn(num, evolved, f'{self.byNum[num][2]} cannot be a starter')

    def test_base_stats_exist_for_all_151(self):
        for num in self.byNum:
            self.assertIn(num, self.STATS, f'missing stats for {self.byNum[num][2]}')
            hp, atk, dfn, spe = self.STATS[num]
            for v in (hp, atk, dfn, spe):
                self.assertTrue(0 < v <= 255, f'stat out of range for {self.byNum[num][2]}: {v}')

    def test_translated_names_match_their_source(self):
        # The Latin ones are drawn with the CP437 font (one byte per character, no
        # lowercase in names); Japanese and Korean use a U8g2 font and are in
        # UTF-8, so they are neither ASCII nor have uppercase letters to
        # check.
        LATIN, CJK = ('fr', 'de'), ('ja', 'ko')
        for num, langs in self.NAMES.items():
            self.assertIn(num, self.byNum, f'translated name for a nonexistent species: {num}')
            for lang, name in langs.items():
                self.assertIn(lang, LATIN + CJK, f'unexpected language: {lang}')
                self.assertTrue(name, f'{num} {lang} is empty')
                if lang in LATIN:
                    self.assertTrue(name.isascii(), f'{num} {lang} is not ASCII: {name}')
                    self.assertEqual(name, name.upper(), f'{num} {lang} has lowercase letters: {name}')
                    self.assertLessEqual(len(name), 12, f'{num} {lang} does not fit on screen: {name}')
                else:
                    # the useful limit is CHARACTERS, not bytes: in UTF-8 each kana or hangul syllable takes 3
                    self.assertLessEqual(len(name), 8, f'{num} {lang} does not fit on screen: {name}')


class TestGeneratedFiles(unittest.TestCase):
    """dex.h is generated: if it does not match its generator, someone edited it by hand."""

    def test_gen_dex_reproduces_dex_h(self):
        tmp = tempfile.mkdtemp(prefix='tamapoke-gen-')
        try:
            shutil.copytree(TOOLS, os.path.join(tmp, 'tools'))
            r = subprocess.run([sys.executable, os.path.join(tmp, 'tools', 'gen_dex.py')],
                               capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, f'gen_dex.py failed:\n{r.stderr}')
            generated = os.path.join(tmp, 'dex.h')
            self.assertTrue(os.path.exists(generated), 'gen_dex.py did not write dex.h')
            with open(generated) as a, open(os.path.join(ROOT, 'dex.h')) as b:
                self.assertEqual(a.read(), b.read(),
                                 'dex.h does not match what tools/gen_dex.py generates '
                                 '(regenerate it instead of editing it by hand)')
            with open(os.path.join(tmp, 'docs', 'EVOLUTION.md'), encoding='utf-8') as a, \
                    open(os.path.join(ROOT, 'docs', 'EVOLUTION.md'), encoding='utf-8') as b:
                self.assertEqual(a.read(), b.read(),
                                 'docs/EVOLUTION.md does not match what tools/gen_dex.py generates')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_generated_files_say_they_are_generated(self):
        for name in ('dex.h', 'species.h'):
            with open(os.path.join(ROOT, name)) as fh:
                head = fh.read(400)
            self.assertIn('GENERATED', head, f'{name} should say it is generated')


class TestSketchSanity(unittest.TestCase):
    """Cheap checks on the .ino that do not need compiling it."""

    @classmethod
    def setUpClass(cls):
        with open(os.path.join(ROOT, 'PokeKnomiGochi.ino')) as fh:
            cls.ino = fh.read()

    def test_sketch_has_no_non_ascii_characters_in_screen_literals(self):
        # comments may contain whatever they like; literals may not:
        # the GFX font is ASCII and an accent comes out as garbage
        for lineno, line in enumerate(self.ino.splitlines(), 1):
            code = line.split('//')[0]
            for chunk in code.split('"')[1::2]:
                self.assertTrue(chunk.isascii(),
                                f'PokeKnomiGochi.ino:{lineno}: non-ASCII literal: {chunk!r}')

    def test_braces_are_balanced(self):
        self.assertEqual(self.ino.count('{'), self.ino.count('}'),
                         'unbalanced braces in PokeKnomiGochi.ino')



class TestHowToPlayCards(unittest.TestCase):
    """Each game's how-to-play card fits: wrapped as drawWrapped() does it in the sketch (at the
    spaces, a number kept with the next word, lines up to CARD_TEXT_W wide), no language needs more
    than six lines between the game's name and the Ready, GO! button, and the button's words fit it. The classic font is 12 px a
    character at size 2; the CJK one (unifont, at 1x there) 8 px for ASCII and 16 for the rest."""

    IDS = ('S_HOW_BERRY', 'S_HOW_BERRY_KEYS', 'S_HOW_WHO', 'S_HOW_RUN', 'S_HOW_DN', 'S_HOW_DN_KEYS',
           'S_HOW_SURF', 'S_HOW_SURF_KEYS')
    MAX_LINES = 6

    @classmethod
    def setUpClass(cls):
        import test_i18n_formats as fmt

        def read(name):
            with open(os.path.join(ROOT, name), encoding='utf-8') as fh:
                return fh.read()

        cls.LANGS = fmt.LANGS
        body = re.search(r'enum\s+StrId[^{]*\{(.*?)\}', read('i18n.h'), re.S).group(1)
        ids = re.findall(r'\b(S_[A-Z0-9_]+)\b', body)
        rows = fmt.extract_table(read('i18n.cpp'), 'STRINGS', len(cls.LANGS))
        cls.LIT = {sid: [row[k] for row in rows] for k, sid in enumerate(ids)}
        ino = read('PokeKnomiGochi.ino')
        cls.MAX_W = int(re.search(r'#define\s+CARD_TEXT_W\s+(\d+)', ino).group(1))
        cls.BTN_W = int(re.search(r'#define\s+CARD_BTN_W\s+(\d+)', ino).group(1))

    @staticmethod
    def chars(literal):
        """The characters of a C literal: an octal escape is one CP437 character."""
        body, out, i = literal[1:-1], [], 0
        while i < len(body):
            if body[i] == '\\':
                m = re.match(r'[0-7]{1,3}', body[i + 1:])
                out.append('?' if m else body[i + 1])
                i += 1 + (len(m.group(0)) if m else 1)
            else:
                out.append(body[i])
                i += 1
        return ''.join(out)

    def width(self, text, cjk):
        if not cjk:
            return 12 * len(text)
        return sum(8 if ord(c) < 0x80 else 16 for c in text)

    def lines(self, text, cjk):
        words = text.split()
        glued = []  # a number keeps to the word after it
        while words:
            w = words.pop(0)
            if w.isdigit() and words:
                w += ' ' + words.pop(0)
            glued.append(w)
        n, line = 0, ''
        for w in glued:
            test = (line + ' ' + w) if line else w
            if line and self.width(test, cjk) > self.MAX_W:
                n, line = n + 1, w
            else:
                line = test
        return n + (1 if line else 0)

    def test_the_button_words_fit(self):
        for li, lang in enumerate(self.LANGS):
            text = self.chars(self.LIT['S_READY_GO'][li])
            cjk = lang in ('JA', 'KO')
            w = self.width(text, cjk) * 2 if cjk else len(text) * 18  # CJK at 2x, the classic font at size 3
            self.assertLessEqual(w, self.BTN_W - 16, f'{lang}: {text}')

    def test_every_card_fits(self):
        for sid in self.IDS:
            self.assertIn(sid, self.LIT)
            for li, lang in enumerate(self.LANGS):
                text = self.chars(self.LIT[sid][li])
                n = self.lines(text, lang in ('JA', 'KO'))
                self.assertLessEqual(n, self.MAX_LINES, f'{sid} {lang}: {n} lines: {text}')

    def test_every_medal_card_fits(self):
        # a medal's card: how to earn it in up to six lines of 336 px under its name and status
        import test_i18n_formats as fmt
        with open(os.path.join(ROOT, 'i18n.cpp'), encoding='utf-8') as fh:
            rows = fmt.extract_table(fh.read(), 'MED_HOW', len(self.LANGS))
        saved = self.MAX_W
        try:
            self.MAX_W = 336
            self.assertEqual(len(rows), len(self.LANGS))
            for li, row in enumerate(rows):  # a row per language, a medal per column
                lang = self.LANGS[li]
                self.assertEqual(len(row), 25)
                for m, lit in enumerate(row):
                    text = self.chars(lit)
                    n = self.lines(text, lang in ('JA', 'KO'))
                    self.assertLessEqual(n, 6, f'medal {m} {lang}: {n} lines: {text}')
        finally:
            self.MAX_W = saved

    def test_no_card_names_a_device(self):
        # the Knomi's cards and the computers' differ in the controls they list, never by naming one
        for sid in self.IDS:
            for li, lang in enumerate(self.LANGS):
                for word in ('knomi', 'mac', 'windows', 'web'):
                    self.assertNotRegex(self.LIT[sid][li].lower(), r'\b' + word + r'\b', f'{sid} {lang}')


class TestTextBuffers(unittest.TestCase):
    """Every snprintf(buf, sizeof(buf), T(S_x), ...) fits in its buffer in ALL languages.

    The buffers were sized when every string was one byte per character. In
    UTF-8 a kana or a hangul syllable takes 3, and snprintf truncates
    silently: sometimes digits are lost ("きろく 2" instead of "きろく 219")
    and sometimes it cuts in the middle of a character, which the font can no
    longer draw. Neither the compiler nor the screen warns about it, so it is
    checked here with the worst case of each argument according to its type:

      %u / %d  -> 5 digits (all arguments are 16 bits or less)
      %lu      -> 10 digits
      %s       -> whatever ARGS_S declares; an undeclared %s makes the
                  test fail, so that whoever adds one thinks about how much it can take
    """

    ARGS_S = {  # what can arrive for each %s, in order
        'S_NAME_FMT': ('star', 'name_or_nick'),
        'S_RELEASE_FMT': ('name',),
        'S_INFO_FMT': ('berry',),
        'S_FAREWELL_BTN': ('name_or_nick',),
        'S_RUNAWAY_BTN': ('name_or_nick',),
        'S_BTL_WILD': ('name',),
        'S_BTL_LOST': ('name',),
        'S_BTL_LEARNED': ('name',),
        'S_ITS_FMT': ('name',),
        'S_CAUGHT_FMT': ('name',),
        'S_FLED_FMT': ('name',),
        'S_VISIT_BYE_FMT': ('name',),
        'S_BUDDY_HERE_FMT': ('name_or_nick',),
        'S_BUDDY_LEFT_FMT': ('name_or_nick',),
        'S_TRADE_GOT_FMT': ('name',),
        'S_UPD_TO_FMT': ('version',),
        'S_BUDDY_INV_PLAY_FMT': ('name_or_nick',),
        'S_SD_THEY_GOT_FMT': ('name_or_nick',),
        'S_SD_LOSE_FMT': ('name_or_nick',),
        'S_GOT_AWAY_FMT': ('name',),
        'S_VB_THEY_SCORE_FMT': ('name_or_nick',),
        'S_SHINY_WILD_FMT': ('name',),
        'S_VISIT_CANDY_FMT': ('name',),
    }
    DIGITS = {'u': 5, 'd': 5, 'lu': 10}
    SPEC = re.compile(r'%0?\d*(lu|u|d|s)')

    @classmethod
    def setUpClass(cls):
        import test_i18n_formats as fmt
        from dex_data import DEX
        from dex_names import LOCAL_NAMES

        def read(name):
            with open(os.path.join(ROOT, name), encoding='utf-8') as fh:
                return fh.read()

        cls.LANGS = fmt.LANGS
        body = re.search(r'enum\s+StrId[^{]*\{(.*?)\}', read('i18n.h'), re.S).group(1)
        ids = re.findall(r'\b(S_[A-Z0-9_]+)\b', body)
        rows = fmt.extract_table(read('i18n.cpp'), 'STRINGS', len(cls.LANGS))
        cls.LIT = {sid: [row[k] for row in rows] for k, sid in enumerate(ids)}
        cls.ino = read('PokeKnomiGochi.ino').splitlines()
        cls.NICK = int(re.search(r'char\s+nick\[(\d+)\]', read('pet.h')).group(1)) - 1
        base = max(len(row[2].encode()) for row in DEX)
        cls.NAME = {}
        for lang in cls.LANGS:
            own = [n[lang.lower()] for n in LOCAL_NAMES.values() if lang.lower() in n]
            cls.NAME[lang] = max([base] + [len(n.encode('utf-8')) for n in own])

    @staticmethod
    def bytes_literal(literal):
        """Bytes a C literal takes in memory, written as in the source."""
        body = literal[1:-1].encode('utf-8')
        n = i = 0
        while i < len(body):
            if body[i] == 0x5C:  # backslash: \ooo octal, or \n \" \\ ...
                j = i + 1
                while j < len(body) and j < i + 4 and 0x30 <= body[j] <= 0x37:
                    j += 1
                i = j if j > i + 1 else i + 2
            else:
                i += 1
            n += 1
        return n

    def worst_case(self, sid, li):
        lang, lit = self.LANGS[li], self.LIT[sid][li]
        total = self.bytes_literal(lit) + 1  # + terminator
        pending = list(self.ARGS_S.get(sid, ()))
        for m in self.SPEC.finditer(lit):
            total -= len(m.group(0))
            if m.group(1) != 's':
                total += self.DIGITS[m.group(1)]
                continue
            self.assertTrue(pending, f'{sid}: has an undeclared %s in ARGS_S')
            arg = pending.pop(0)
            if arg == 'star':
                total += 1
            elif arg == 'name':
                total += self.NAME[lang]
            elif arg == 'name_or_nick':
                total += max(self.NAME[lang], self.NICK)
            elif arg == 'version':
                total += 15  # otaNewVersion(): newVer[16]
            elif arg == 'berry':
                total += max(self.bytes_literal(self.LIT[b][li]) for b in
                             ('S_BERRY_UNK', 'S_BERRY_RED', 'S_BERRY_BLUE', 'S_BERRY_GREEN'))
        return total

    def test_every_formatted_text_fits_in_its_buffer_in_all_languages(self):
        call = re.compile(r'snprintf\((\w+),\s*sizeof\(\1\),\s*T\((S_[A-Z0-9_]+)\)')
        checked = 0
        for n, line in enumerate(self.ino, 1):
            m = call.search(line.split('//')[0])
            if not m:
                continue
            var, sid = m.groups()
            size = next((int(d.group(1)) for k in range(n - 2, -1, -1)
                         for d in [re.search(r'\bchar\s+' + var + r'\s*\[(\d+)\]', self.ino[k])]
                         if d), None)
            self.assertIsNotNone(size, f'PokeKnomiGochi.ino:{n}: cannot find the declaration of {var}')
            checked += 1
            for li, lang in enumerate(self.LANGS):
                with self.subTest(line=n, id=sid, language=lang):
                    worst = self.worst_case(sid, li)
                    self.assertLessEqual(
                        worst, size,
                        f'PokeKnomiGochi.ino:{n}: {var}[{size}] with {sid} in {lang} needs {worst} bytes')
        self.assertGreater(checked, 10, 'the snprintf pattern no longer finds the calls')

if __name__ == '__main__':
    unittest.main(verbosity=2)
