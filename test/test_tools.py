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
