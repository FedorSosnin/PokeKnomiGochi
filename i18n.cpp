#include "i18n.h"
#include "pet.h"        // MED_COUNT
#include <Preferences.h>

Lang gLang = LANG_DEFAULT;

// String table [language][id]. No accents or enyes: the firmware's bitmap font
// doesn't have them (that's why Spanish already went "Esta", "bano", etc.).
static const char *const STRINGS[LANG_COUNT][STR_COUNT] = {
  // ---------------- ES ----------------
  {
    "Est\240 evolucionando!", "Nam nam!", "Le gusta!", "Tiene hambre!", "Necesita un ba\244o!",
    "Est\240 agotado...", "Est\240 triste...", "Est\240 rellenito...", "Es SHINY!!", "Est\240 feliz",
    "GRACIAS! Hasta siempre", "Se ha escapado...", "Adi\242s! Se despide...",
    "HUEVO", "Huevo legendario!?", "Huevo raro!", "Toca el huevo...", "Se mueve!", "Est\240 a punto!",
    "POKEDEX %u/151",
    "%s%s Nv.%u",
    "Soltar a %s?", "SI", "NO",
    "VICTORIAS: %u", "FUERZA +%u", "NUEVO RECORD!", "RECORD: %u", "HUIR",
    "PUNTOS: %u", "Qu\202 felicidad!", "+felicidad",
    "AJUSTAR HORA", "HORA", "MIN", "desliza arriba: cancelar", "Idioma",
    "MEDALLA!", "GENIAL!", "RACHA %u DIAS!",
    "RACHA %u  rec %u", "VIN", "BAYA ???", "BAYA FRAMBU", "BAYA ARANJA", "BAYA PINIA",
    "%s   EDAD %lud", "toca el nombre: renombrar",
    "ESTADISTICAS", "FUE", "DEF", "VEL", "PES", "COMBATIR!",
    "MEDALLAS %d/%d", "toca: volver",
    "NOMBRE:", "toca para volver",
    "COM", "FEL", "ENE", "LIM",
    "REC %u",
    "PROGRESO", "Nv.%u", "%u min para Nv.%u", "EVOLUCION", "Forma final",
    "Listo para evolucionar!", "Sube todo a 40 para evolucionar",
    "Evoluciona en %u niv.", "Descuidos: %u",
    "SON ON", "SON OFF",
    "EVOLUCIONAR", "%s quiere decirte algo...", "%s se siente abandonado...",
    "Evolucionar?", "Mantener forma", "Despedirse?", "Despedirse", "Quedaros juntos",
    "Elige tu inicial",
    "Sin sprites", "C\240rgalos en la SD",
    "%s salvaje!", "Muy eficaz!", "Poco eficaz...", "No le afecta!", "HAS GANADO!", "%s se debilit\242...", "Has escapado", "Nuevo en POKEDEX!", "Aprendi\242 %s!", "Muy cansado!", "Empezar de nuevo?", "BORRAR TODO", "SEGUIR",
    "AJUSTES", "CAMBIAR WI-FI", "Hora sincronizada", "Sin conexi\242n", "Buscando redes...", "Conectando...", "Guardado!", "Fallo: sigue la anterior", "CANCELAR",
    "JUEGOS", "Inclina y Atrapa", "Qui\202n es?", "Qui\202n es ese Pokemon?", "Es %s!", "Inclina para moverte!", "Toca y apunta inclinando", "%s atrapado!", "Fallo!", "%s escap\242...", "Tienes una visita!", "Adi\242s, %s!", "PokeRun", "Toca para saltar!", "Desliza arriba: salir", "Elige tu Pokemon", "Elegir", "Volver", "Te necesita!", "Cuidarlo", "Dejarlo ir", "PokeDungeon", "Baya! +PS", "%s esta aqui!", "Combatir", "Intercambiar", "%s quiere combatir!", "%s quiere cambiar!", "Esperando...", "%s se fue", "%s no puede jugar", "Elige cual dar", "Das", "Recibes", "Tienes a %s!",
    "Combate", "Bosque", "Playa", "Cueva", "Mansi\242n", "Campo", "Buscar", "%d/%d atrapados", "Sin atrapar", "Desliza arriba: volver",
    "ENTRENADOR %d/%d",
    "BUSCAR ACTUALIZACION", "Ya est\240 al d\241a", "ACTUALIZAR A v%s", "Actualizando %u%%", "Fall\242 la actualizaci\242n", "%u amigos aqu\241!",
    "Jugar", "Jugar juntos", "Duelo: Qui\202n es?", "%s quiere jugar!", "Prep\240rate...", "T\243", "Lo tienes!", "%s lo tiene!", "Nadie acert\242", "Ganaste!", "Gana %s!", "Empate!", "%s se escap\242!", "Voley Poke Ball", "Tu saque!", "Punto!", "Punto para %s!", "Inclina o arrastra",
    "ZONAS %d/%d", "%s variocolor!",
  },
  // ---------------- EN ----------------
  {
    "Evolving!", "Yum yum!", "It likes it!", "It's hungry!", "Needs a bath!",
    "Worn out...", "Feeling sad...", "A bit chubby...", "It's SHINY!!", "It's happy",
    "THANKS! Farewell", "It ran away...", "Bye! Waving goodbye...",
    "EGG", "Legendary egg!?", "Rare egg!", "Tap the egg...", "It moves!", "Almost there!",
    "POKEDEX %u/151",
    "%s%s Lv.%u",
    "Release %s?", "YES", "NO",
    "WINS: %u", "STR +%u", "NEW RECORD!", "BEST: %u", "RUN",
    "SCORE: %u", "So much fun!", "+happiness",
    "SET TIME", "HOUR", "MIN", "swipe up: cancel", "Lang",
    "MEDAL!", "AWESOME!", "%u DAY STREAK!",
    "STREAK %u  best %u", "BOND", "BERRY ???", "RAZZ BERRY", "ORAN BERRY", "PINAP BERRY",
    "%s   AGE %lud", "tap name: rename",
    "STATS", "ATK", "DEF", "SPD", "WGT", "FIGHT!",
    "MEDALS %d/%d", "tap: back",
    "NAME:", "tap to go back",
    "FOOD", "JOY", "ENE", "HYG",
    "BEST %u",
    "PROGRESS", "Lv.%u", "%u min to Lv.%u", "EVOLUTION", "Final form",
    "Ready to evolve!", "All needs >=40 to evolve",
    "Evolves in %u lv.", "Slip-ups: %u",
    "SND ON", "SND OFF",
    "EVOLVE!", "%s wants to tell you...", "%s feels abandoned...",
    "Evolve?", "Keep form", "Say goodbye?", "Goodbye", "Stay together",
    "Choose your starter",
    "No sprites", "Load them onto the SD",
    "Wild %s!", "Super effective!", "Not very effective", "No effect!", "YOU WON!", "%s fainted...", "Got away safely", "New in POKEDEX!", "Learned %s!", "Too tired!", "Start over?", "ERASE EVERYTHING", "KEEP PLAYING",
    "SETTINGS", "CHANGE WI-FI", "Time synced", "Can't connect", "Searching...", "Connecting...", "Saved!", "Failed: old one kept", "CANCEL",
    "GAMES", "Tilt Catch", "Who's That?", "Who's that Pokemon?", "It's %s!", "Lean to move!", "Tap ball, lean to aim", "Caught %s!", "Missed!", "%s got away...", "You have a visitor!", "Goodbye, %s!", "PokeRun", "Tap to jump!", "Swipe up to quit", "Choose your Pokemon", "Choose", "Back", "It needs you!", "Take care of it", "Let it go", "PokeDungeon", "Berry! +HP", "%s is here!", "Battle", "Trade", "%s wants to battle!", "%s wants to trade!", "Waiting...", "%s left", "%s can't play now", "Pick one to give", "You give", "You get", "Got %s!",
    "Battle", "Forest", "Beach", "Cave", "Mansion", "Field", "Search", "%d/%d caught", "Not caught yet", "Swipe up to go back",
    "TRAINER %d/%d",
    "CHECK FOR UPDATE", "Up to date", "UPDATE TO v%s", "Updating %u%%", "Update failed", "%u buddies are here!",
    "Play", "Play together", "Who's That? Showdown", "%s wants to play!", "Get ready...", "You", "You got it!", "%s got it!", "Nobody got it", "You win!", "%s wins!", "It's a draw!", "%s got away!", "Poke Ball Volley", "Your serve!", "Point!", "%s scores!", "Tilt or drag to move",
    "ZONES %d/%d", "Shiny %s!",
  },
  // ---------------- FR ----------------
  {
    "Il \202volue!", "Miam miam!", "Il aime \207a!", "Il a faim!", "Besoin d'un bain!",
    "\220puis\202...", "Triste...", "Un peu rond...", "C'est SHINY!!", "Il est content",
    "MERCI! Adieu", "Il s'est enfui...", "Au revoir!",
    "OEUF", "Oeuf l\202gendaire!?", "Oeuf rare!", "Touche l'oeuf...", "Il bouge!", "Presque l\205!",
    "POKEDEX %u/151",
    "%s%s Niv.%u",
    "Rel\203cher %s?", "OUI", "NON",
    "VICTOIRES: %u", "FORCE +%u", "NOUVEAU RECORD!", "RECORD: %u", "FUITE",
    "SCORE: %u", "Trop bien!", "+bonheur",
    "R\220GLER L'HEURE", "HEURE", "MIN", "glisse haut: annuler", "Langue",
    "M\220DAILLE!", "SUPER!", "S\220RIE %u JOURS!",
    "S\220RIE %u  rec %u", "LIEN", "BAIE ???", "BAIE FRAMBY", "BAIE ORAN", "BAIE NANANA",
    "%s   AGE %lud", "touche le nom: renommer",
    "STATS", "ATQ", "DEF", "VIT", "PDS", "COMBAT!",
    "M\220DAILLES %d/%d", "touche: retour",
    "NOM:", "touche pour revenir",
    "NOUR", "JOIE", "ENE", "HYG",
    "REC %u",
    "PROGRES", "Niv.%u", "%u min pour Niv.%u", "\220VOLUTION", "Forme finale",
    "Pr\210t \205 \202voluer!", "Tout \205 40 pour \202voluer",
    "\220volue dans %u niv.", "N\202gligences: %u",
    "SON ON", "SON OFF",
    "\220VOLUER", "%s veut te parler...", "%s se sent abandonn\202...",
    "\220voluer?", "Garder forme", "Dire adieu?", "Adieu", "Rester ensemble",
    "Choisis ton starter",
    "Pas de sprites", "Charge-les sur la SD",
    "%s sauvage!", "Super efficace!", "Pas tr\212s efficace", "Sans effet!", "VICTOIRE!", "%s est K.O.", "Fuite r\202ussie", "Nouveau POKEDEX!", "Apprend %s!", "Trop fatigu\202!", "Recommencer?", "TOUT EFFACER", "CONTINUER",
    "REGLAGES", "CHANGER LE WI-FI", "Heure synchronis\202e", "Pas de connexion", "Recherche...", "Connexion...", "Enregistr\202!", "Echec: ancien conserv\202", "ANNULER",
    "JEUX", "Penche et Attrape", "Qui est-ce?", "Quel est ce Pokemon?", "C'est %s!", "Penche pour bouger!", "Touche, penche pour viser", "%s attrap\202!", "Rat\202!", "%s s'enfuit...", "Tu as un visiteur!", "Au revoir, %s!", "PokeRun", "Touche pour sauter!", "Glisse en haut: quitter", "Choisis ton Pokemon", "Choisir", "Retour", "Il a besoin de toi!", "M'en occuper", "Le laisser partir", "PokeDungeon", "Baie! +PV", "%s est l\205!", "Combattre", "Echanger", "%s veut combattre!", "%s veut \202changer!", "Attente...", "%s est parti", "%s ne peut pas jouer", "Choisis quoi donner", "Tu donnes", "Tu re\207ois", "%s obtenu!",
    "Combat", "For\210t", "Plage", "Grotte", "Manoir", "Champ", "Chercher", "%d/%d captur\202s", "Pas encore captur\202", "Glisse en haut: retour",
    "DRESSEUR %d/%d",
    "MISE \205 JOUR ?", "D\202j\205 \205 jour", "INSTALLER v%s", "Mise \205 jour %u%%", "\220chec de mise \205 jour", "%u copains sont l\205 !",
    "Jouer", "Jouer ensemble", "Duel: Qui est-ce?", "%s veut jouer!", "Pr\202pare-toi...", "Toi", "Trouv\202!", "%s a trouv\202!", "Personne n\'a trouv\202", "Gagn\202!", "%s gagne!", "Egalit\202!", "%s s\'est enfui!", "Volley Poke Ball", "A toi de servir!", "Point!", "Point pour %s!", "Penche ou glisse",
    "ZONES %d/%d", "%s chromatique!",
  },
  // ---------------- DE ----------------
  // German does take umlauts: the firmware's 5x7 font is a full CP437 table
  // (256 glyphs), so the single byte of each character is enough.
  // They are in octal so as not to depend on the source file's encoding:
  //   \204 a   \224 o   \201 u   \216 A   \231 O   \232 U   \341 ss
  // One byte per character: strlen() still measures correctly and centering doesn't change.
  {
    "Entwickelt sich!", "Mampf mampf!", "Gef\204llt ihm!", "Hat Hunger!", "Braucht ein Bad!",
    "Ersch\224pft...", "Traurig...", "Etwas rundlich...", "Es ist SHINY!!", "Es ist froh",
    "DANKE! Leb wohl", "Es ist weg...", "Tsch\201ss! Winkt",
    "EI", "Legend\204res Ei!?", "Seltenes Ei!", "Ber\201hre das Ei...", "Es bewegt sich!", "Fast so weit!",
    "POKEDEX %u/151",
    "%s%s Lv.%u",
    "%s freilassen?", "JA", "NEIN",
    "SIEGE: %u", "KRAFT +%u", "NEUER REKORD!", "REKORD: %u", "FLUCHT",
    "PUNKTE: %u", "Wie sch\224n!", "+Freude",
    "ZEIT STELLEN", "STD", "MIN", "hoch wischen: Abbruch", "Sprache",
    "MEDAILLE!", "TOLL!", "%u TAGE SERIE!",
    "SERIE %u  rek %u", "BND", "BEERE ???", "HIMMIHBEERE", "SINELBEERE", "SANANABEERE",
    "%s   ALTER %lud", "Name tippen: umbenennen",
    "WERTE", "ANG", "VER", "INI", "GEW", "KAMPF!",
    "MEDAILLEN %d/%d", "tippen: zur\201ck",
    "NAME:", "tippen: zur\201ck",
    "ESS", "FRO", "ENE", "HYG",
    "REK %u",
    "FORTSCHRITT", "Lv.%u", "%u min bis Lv.%u", "ENTWICKLUNG", "Endform",
    "Bereit zur Entwicklung!", "Alles >=40 zur Entwicklung",
    "Entwicklung in %u Lv.", "Patzer: %u",
    "TON AN", "TON AUS",
    "ENTWICKELN", "%s will dir etwas sagen...", "%s f\201hlt sich verlassen...",
    "Entwickeln?", "Form behalten", "Abschied?", "Leb wohl", "Zusammen bleiben",
    "W\204hle deinen Starter",
    "Keine Sprites", "Auf die SD laden",
    "Wildes %s!", "Sehr effektiv!", "Nicht sehr effektiv", "Keine Wirkung!", "GEWONNEN!", "%s ist K.O.", "Entkommen!", "Neu im POKEDEX!", "Lernt %s!", "Zu m\201de!", "Neu anfangen?", "ALLES LOESCHEN", "WEITERSPIELEN",
    "EINSTELLUNGEN", "WLAN WECHSELN", "Zeit synchron", "Keine Verbindung", "Suche...", "Verbinde...", "Gespeichert!", "Fehler: altes bleibt", "ABBRECHEN",
    "SPIELE", "Neigen und Fangen", "Wer ist das?", "Wer ist das Pokemon?", "Es ist %s!", "Neigen zum Bewegen!", "Tippen, neigen zum Zielen", "%s gefangen!", "Daneben!", "%s ist entkommen", "Du hast Besuch!", "Tschuess, %s!", "PokeRun", "Tippen zum Springen!", "Hochwischen: Ende", "W\204hle dein Pokemon", "W\204hlen", "Zur\201ck", "Es braucht dich!", "K\201mmern", "Gehen lassen", "PokeDungeon", "Beere! +KP", "%s ist da!", "Kampf", "Tausch", "%s will k\204mpfen!", "%s will tauschen!", "Warten...", "%s ist weg", "%s kann nicht spielen", "W\204hle zum Geben", "Du gibst", "Du kriegst", "%s erhalten!",
    "Kampf", "Wald", "Strand", "H\224hle", "Spukhaus", "Wiese", "Suchen", "%d/%d gefangen", "Noch nicht gefangen", "Hochwischen: zur\201ck",
    "TRAINER %d/%d",
    "NACH UPDATE SUCHEN", "Aktuell", "UPDATE AUF v%s", "Update l\204uft %u%%", "Update fehlgeschlagen", "%u Kumpel sind da!",
    "Spielen", "Zusammen spielen", "Duell: Wer ist das?", "%s will spielen!", "Mach dich bereit...", "Du", "Richtig!", "%s war schneller!", "Niemand wusste es", "Gewonnen!", "%s gewinnt!", "Unentschieden!", "%s ist entkommen!", "Poke-Ball-Volley", "Du schl\204gst auf!", "Punkt!", "Punkt f\201r %s!", "Neigen oder ziehen",
    "ZONEN %d/%d", "Schillerndes %s!",
  },
  // ---------------- IT ----------------
  {
    "Si evolve!", "Gnam gnam!", "Gli piace!", "Ha fame!", "Vuole un bagno!",
    "Esausto...", "Triste...", "Un po' cicciotto...", "E' SHINY!!", "E' felice",
    "GRAZIE! Addio", "E' scappato...", "Ciao! Saluta",
    "UOVO", "Uovo leggendario!?", "Uovo raro!", "Tocca l'uovo...", "Si muove!", "Ci siamo quasi!",
    "POKEDEX %u/151",
    "%s%s Lv.%u",
    "Liberare %s?", "SI", "NO",
    "VITTORIE: %u", "FORZA +%u", "NUOVO RECORD!", "RECORD: %u", "FUGA",
    "PUNTI: %u", "Che gioia!", "+felicit\205",
    "IMPOSTA ORA", "ORA", "MIN", "scorri su: annulla", "Lingua",
    "MEDAGLIA!", "GRANDE!", "SERIE %u GIORNI!",
    "SERIE %u  rec %u", "LEG", "BACCA ???", "BACCALAMPON", "BACCARANCIA", "BACCANANAS",
    "%s   ETA %lud", "tocca il nome: rinomina",
    "STATISTICHE", "ATT", "DIF", "VEL", "PES", "LOTTA!",
    "MEDAGLIE %d/%d", "tocca: indietro",
    "NOME:", "tocca per tornare",
    "CIB", "GIO", "ENE", "IGI",
    "REC %u",
    "PROGRESSI", "Lv.%u", "%u min per Lv.%u", "EVOLUZIONE", "Forma finale",
    "Pronto a evolvere!", "Tutto a 40 per evolvere",
    "Evolve tra %u liv.", "Disattenzioni: %u",
    "AUD ON", "AUD OFF",
    "EVOLVI", "%s vuole dirti qualcosa...", "%s si sente abbandonato...",
    "Evolvere?", "Mantieni forma", "Salutare?", "Addio", "Restare insieme",
    "Scegli l'iniziale",
    "Senza sprite", "Caricali sulla SD",
    "%s selvatico!", "Superefficace!", "Non molto efficace", "Nessun effetto!", "HAI VINTO!", "%s \212 esausto...", "Fuga riuscita", "Nuovo nel POKEDEX!", "Impara %s!", "Troppo stanco!", "Ricominciare?", "CANCELLA TUTTO", "CONTINUA",
    "IMPOSTAZIONI", "CAMBIA WI-FI", "Ora sincronizzata", "Nessuna connessione", "Ricerca...", "Connessione...", "Salvato!", "Errore: resta il vecchio", "ANNULLA",
    "GIOCHI", "Inclina e Prendi", "Chi \212?", "Chi \212 questo Pokemon?", "E' %s!", "Inclina per muoverti!", "Tocca, inclina per mirare", "%s catturato!", "Mancato!", "%s \212 fuggito", "Hai una visita!", "Ciao, %s!", "PokeRun", "Tocca per saltare!", "Scorri su: esci", "Scegli il tuo Pokemon", "Scegli", "Indietro", "Ha bisogno di te!", "Prenditene cura", "Lascialo andare", "PokeDungeon", "Bacca! +PS", "%s \212 qui!", "Lotta", "Scambio", "%s vuole lottare!", "%s vuole scambiare!", "Attesa...", "%s se n'\212 andato", "%s non pu\225 giocare", "Scegli cosa dare", "Dai", "Ricevi", "Preso %s!",
    "Lotta", "Foresta", "Spiaggia", "Grotta", "Villa", "Prato", "Cerca", "%d/%d presi", "Non ancora preso", "Scorri su: indietro",
    "ALLENATORE %d/%d",
    "CERCA AGGIORNAMENTI", "Gi\205 aggiornato", "AGGIORNA A v%s", "Aggiornamento %u%%", "Aggiornamento fallito", "%u amici sono qui!",
    "Gioca", "Giochiamo insieme", "Sfida: Chi \212?", "%s vuole giocare!", "Pronti...", "Tu", "Indovinato!", "%s ha indovinato!", "Nessuno l\'ha preso", "Hai vinto!", "Vince %s!", "Pareggio!", "%s \212 scappato!", "Pallavolo Poke Ball", "Battuta tua!", "Punto!", "Punto a %s!", "Inclina o trascina",
    "ZONE %d/%d", "%s cromatico!",
  },
  // ---------------- PT ----------------
  {
    "Evoluindo!", "Nham nham!", "Ele gosta!", "Est\240 com fome!", "Precisa de banho!",
    "Exausto...", "Triste...", "Um pouco gordinho...", "\220 SHINY!!", "Est\240 feliz",
    "OBRIGADO! Adeus", "Fugiu...", "Tchau! Acena",
    "OVO", "Ovo lend\240rio!?", "Ovo raro!", "Toque no ovo...", "Mexe-se!", "Quase l\240!",
    "POKEDEX %u/151",
    "%s%s Niv.%u",
    "Soltar %s?", "SIM", "NAO",
    "VITORIAS: %u", "FOR\200A +%u", "NOVO RECORDE!", "RECORDE: %u", "FUGIR",
    "PONTOS: %u", "Que alegria!", "+alegria",
    "AJUSTAR HORA", "HORA", "MIN", "deslize cima: cancelar", "Idioma",
    "MEDALHA!", "OTIMO!", "%u DIAS SEGUIDOS!",
    "SEQ %u  rec %u", "LA\200O", "BAGA ???", "BAGA FRAMBO", "BAGA ORAN", "BAGA PINAP",
    "%s   IDADE %lud", "toque no nome: renomear",
    "ESTATISTICAS", "ATQ", "DEF", "VEL", "PES", "LUTAR!",
    "MEDALHAS %d/%d", "toque: voltar",
    "NOME:", "toque para voltar",
    "COM", "ALE", "ENE", "HIG",
    "REC %u",
    "PROGRESSO", "Niv.%u", "%u min para Niv.%u", "EVOLUCAO", "Forma final",
    "Pronto a evoluir!", "Tudo a 40 para evoluir",
    "Evolui em %u niv.", "Descuidos: %u",
    "SOM ON", "SOM OFF",
    "EVOLUIR", "%s quer dizer-te algo...", "%s sente-se abandonado...",
    "Evoluir?", "Manter forma", "Despedir?", "Adeus", "Ficar juntos",
    "Escolhe o inicial",
    "Sem sprites", "Carrega-os no SD",
    "%s selvagem!", "Super eficaz!", "Pouco eficaz...", "Sem efeito!", "VENCESTE!", "%s desmaiou...", "Fugiste!", "Novo na POKEDEX!", "Aprendeu %s!", "Muito cansado!", "Recome\207ar?", "APAGAR TUDO", "CONTINUAR",
    "DEFINICOES", "MUDAR WI-FI", "Hora sincronizada", "Sem rede", "A procurar...", "Ligando...", "Guardado!", "Erro: mantem o antigo", "CANCELAR",
    "JOGOS", "Inclina e Apanha", "Quem \202?", "Quem \202 este Pokemon?", "E o %s!", "Inclina para mover!", "Toca e inclina para mirar", "%s apanhado!", "Falhou!", "%s fugiu...", "Tens uma visita!", "Adeus, %s!", "PokeRun", "Toca para saltar!", "Desliza acima: sair", "Escolhe o teu Pokemon", "Escolher", "Voltar", "Precisa de ti!", "Cuidar dele", "Deixar ir", "PokeDungeon", "Baga! +PS", "%s est\240 aqui!", "Combater", "Trocar", "%s quer combater!", "%s quer trocar!", "A esperar...", "%s saiu", "%s nao pode jogar", "Escolhe o que dar", "Das", "Recebes", "Tens %s!",
    "Combate", "Floresta", "Praia", "Caverna", "Solar", "Campo", "Procurar", "%d/%d apanhados", "Ainda por apanhar", "Desliza acima: voltar",
    "TREINADOR %d/%d",
    "PROCURAR ATUALIZACAO", "J\240 est\240 atualizado", "ATUALIZAR PARA v%s", "A atualizar %u%%", "Falhou a atualizacao", "%u amigos aqui!",
    "Jogar", "Jogar juntos", "Duelo: Quem \202?", "%s quer jogar!", "Prepara-te...", "Tu", "Acertaste!", "%s acertou!", "Ningu\202m acertou", "Ganhaste!", "%s ganha!", "Empate!", "%s escapou!", "Voleibol Poke Ball", "Tu serves!", "Ponto!", "Ponto para %s!", "Inclina ou arrasta",
    "ZONAS %d/%d", "%s brilhante!",
  },
  // ---------------- JA ----------------
  {
    "しんかした！", "おいしい！", "うれしそう！", "おなかすいた！", "おふろにいれて！",
    "つかれた...", "かなしい...", "ちょっとふとった...", "いろちがい！！", "ごきげん！",
    "ありがとう！ またね", "にげちゃった...", "バイバイ！",

    "たまご", "でんせつのたまご！？", "めずらしいたまご！", "たまごをタッチ...", "うごいた！", "もうすぐ！",

    "ずかん %u/151",

    "%s%s Lv.%u",

    "%s とおわかれ？", "はい", "いいえ",

    "しょうり: %u", "ちから +%u", "しんきろく！", "きろく: %u", "にげる",

    "スコア: %u", "たのしい！", "+ごきげん",

    "じかんせってい", "じ", "ふん", "うえスワイプ: もどる", "ことば",

    "メダル！", "すごい！", "%u にちれんぞく！",

    "れんぞく %u  きろく %u", "なかよし", "きのみ ???", "ズリのみ", "オレンのみ", "パイルのみ",

    "%s   %luにち", "なまえをタッチ: へんこう",

    "ステータス", "こうげき", "ぼうぎょ", "すばやさ", "たいじゅう", "たたかう!",

    "メダル %d/%d", "タッチ: もどる",

    "なまえ:", "タッチでもどる",

    "ごはん", "ごきげん", "げんき", "きれい",

    "きろく %u",

    "せいちょう", "Lv.%u", "あと%uふんで Lv.%u", "しんか", "さいしゅうけい",

    "しんかできる！", "ぜんぶ40でしんか",

    "%u Lv.でしんか", "おせわミス: %u",

    "おと", "おと",

    "しんか", "%sがなにかいいたそう...", "%sがさびしがっている...",

    "しんかする？", "このまま", "おわかれする？", "さようなら", "いっしょにいる",

    "さいしょのポケモンをえらぶ",

    "スプライトなし", "SDにいれてください",
    "やせいの%s!", "こうかばつぐん!", "いまひとつ...", "こうかなし!", "かった!", "%sはたおれた", "にげられた", "ずかんにとうろく!", "%sをおぼえた!", "つかれている!", "さいしょから?", "ぜんぶけす", "つづける",
    "せってい", "WI-FIへんこう", "じこくOK", "せつぞくなし", "さがしています...", "せつぞくちゅう...", "ほぞんした!", "しっぱい:まえのまま", "キャンセル",
    "ゲーム", "かたむけキャッチ", "だれだ?", "このポケモンはだれ?", "%sだ!", "かたむけてうごこう!", "タッチしてかたむける", "%sをつかまえた!", "はずれ!", "%sはにげた", "おきゃくさんだよ!", "またね %s!", "PokeRun", "タッチでジャンプ!", "うえにスワイプでやめる", "ポケモンをえらぼう", "えらぶ", "もどる", "きみがひつようだよ!", "おせわする", "おわかれする", "PokeDungeon", "きのみ! HPかいふく", "%sがきたよ!", "バトル", "こうかん", "%sがバトルしたいって!", "%sがこうかんしたいって!", "まってね...", "%sはかえった", "%sはあそべないよ", "あげるのをえらぼう", "あげる", "もらう", "%sをもらった!",
    "バトル", "もり", "うみべ", "どうくつ", "やかた", "のはら", "さがす", "%d/%d つかまえた", "まだつかまえてない", "うえにスワイプでもどる",
    "トレーナー %d/%d",
    "アップデート", "さいしんです", "v%s にする", "アップデート %u%%", "しっぱいしました", "ともだち %u にん!",
    "あそぶ", "いっしょに あそぶ", "だれだ? たいけつ", "%sが あそびたいって!", "よーい...", "きみ", "せいかい!", "%sが せいかい!", "だれも わからなかった", "きみの かち!", "%sの かち!", "ひきわけ!", "%sに にげられた!", "ボール バレー", "きみの サーブ!", "とくてん!", "%sの とくてん!", "かたむけるか ドラッグ",
    "ゾーン %d/%d", "いろちがいの%s!",
  },
  // ---------------- KO ----------------
  // The specifiers go in the SAME ORDER as in the English row even though
  // natural Korean would place them differently (e.g. S_NEXT_LVL_FMT):
  // T(id) is filled with varargs in a fixed order at each call site, so
  // reordering %u/%s here without touching the call site would silently swap
  // which value lands in which slot.
  {
    "진화 중!", "냠냠!", "마음에 들어해요!", "배고파요!", "목욕이 필요해요!",
    "지쳤어요...", "슬퍼해요...", "조금 통통해요...", "이로치예요!!", "행복해요",
    "고마워요! 안녕", "가출했어요...", "잘 가! 손을 흔들며...",
    "알", "전설의 알!?", "희귀한 알!", "알을 톡톡...", "움직여요!", "거의 다 됐어요!",
    "도감 %u/151",
    "%s%s Lv.%u",
    "%s 놓아줄까요?", "예", "아니오",
    "승리: %u", "힘 +%u", "신기록!", "최고: %u", "도망",
    "점수: %u", "정말 즐거워요!", "+행복",
    "시간 설정", "시", "분", "위로 스와이프: 취소", "언어",
    "메달!", "최고예요!", "%u일 연속!",
    "연속 %u  최고 %u", "유대감", "열매 ???", "라즈열매", "오랭열매", "파인열매",
    "%s   나이 %lud", "이름 탭: 이름변경",
    "능력치", "공격", "방어", "속도", "무게", "배틀!",
    "메달 %d/%d", "탭: 뒤로",
    "이름:", "탭하면 뒤로",
    "포만", "기분", "기력", "청결",
    "최고 %u",
    "진행도", "Lv.%u", "%u분 후 Lv.%u", "진화", "최종 형태",
    "진화 준비 완료!", "진화하려면 전부 40 이상",
    "%u레벨 이후에 진화", "실수: %u",
    "소리 켬", "소리 끔",
    "진화!", "%s가 할 말이 있대요...", "%s가 버려진 기분이래요...",
    "진화할까요?", "형태 유지", "작별할까요?", "작별", "계속 함께",
    "스타터를 선택하세요",
    "스프라이트 없음", "SD 카드에 넣어주세요",
    "야생 %s!", "효과가 굉장했다!", "효과가 별로다", "효과 없음!", "승리!", "%s 쓰러짐", "도망쳤다", "도감 등록!", "%s 배움!", "너무 지쳤다!", "처음부터?", "모두 지우기", "계속하기",
    "설정", "WI-FI 변경", "시간 동기화됨", "연결 안 됨", "검색 중...", "연결 중...", "저장됨!", "실패: 이전 유지", "취소",
    "게임", "기울여 잡기", "누구일까?", "이 포켓몬은 누구?", "%s!", "기울여서 움직여요!", "눌러서 기울여 조준", "%s 잡았다!", "빗나감!", "%s 도망감", "손님이 왔어요!", "잘 가, %s!", "PokeRun", "눌러서 점프!", "위로 밀면 나가기", "포켓몬을 고르세요", "고르기", "돌아가기", "널 필요로 해요!", "돌봐 주기", "떠나보내기", "PokeDungeon", "열매! HP 회복", "%s 왔어요!", "배틀", "교환", "%s: 배틀할래요!", "%s: 교환할래요!", "기다리는 중...", "%s 떠났어요", "%s 지금 못 놀아요", "줄 포켓몬 고르기", "줄래요", "받아요", "%s 받았어요!",
    "배틀", "숲", "해변", "동굴", "저택", "들판", "찾기", "%d/%d 잡음", "아직 못 잡음", "위로 밀면 뒤로",
    "트레이너 %d/%d",
    "업데이트 확인", "최신 버전", "v%s 로 업데이트", "업데이트 중 %u%%", "업데이트 실패", "친구 %u명이 왔어요!",
    "놀기", "함께 놀기", "누구일까? 대결", "%s: 같이 놀래요!", "준비...", "나", "정답!", "%s 정답!", "아무도 못 맞췄어요", "내가 이겼다!", "%s 승리!", "무승부!", "%s: 도망쳤다!", "몬스터볼 배구", "내 서브!", "득점!", "%s 득점!", "기울이거나 끌기",
    "지역 %d/%d", "이로치 %s!",
  },
};

// Medal names in their three lengths [language][medal].
static const char *const MED_NAME[LANG_COUNT][MED_COUNT] = {
  { "Nv.10", "Nv.25", "Nv.50", "BAYA", "RACHA 7", "VINCULO", "FORMA TOPE", "EN FORMA",
    "VARIOCOLOR", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Nv.100", "AMIGO", "FIESTA", "JUGAR", "BOSQUE", "PLAYA", "CUEVA", "MANSION", "CAMPO" },
  { "Lv.10", "Lv.25", "Lv.50", "BERRY", "7 STREAK", "BOND", "TOP FORM", "IN SHAPE",
    "SHINY!", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Lv.100", "BUDDY", "PARTY", "PLAYMATE", "FOREST", "BEACH", "CAVE", "MANSION", "FIELD" },
  { "Niv.10", "Niv.25", "Niv.50", "BAIE", "SERIE 7", "LIEN", "FORME MAX", "EN FORME",
    "CHROMATIQUE", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Niv.100", "COPAIN", "FETE", "JOUER", "FORET", "PLAGE", "GROTTE", "MANOIR", "CHAMP" },
  { "Lv.10", "Lv.25", "Lv.50", "BEERE", "7 SERIE", "BINDUNG", "ENDFORM", "FIT",
    "SCHILLERND", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Lv.100", "KUMPEL", "PARTY", "SPIELEN", "WALD", "STRAND", "HOEHLE", "SPUKHAUS", "WIESE" },
  { "Lv.10", "Lv.25", "Lv.50", "BACCA", "SERIE 7", "LEGAME", "FORMA MAX", "IN FORMA",
    "CROMATICO", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Lv.100", "AMICO", "FESTA", "GIOCARE", "FORESTA", "SPIAGGIA", "GROTTA", "VILLA", "PRATO" },
  { "Niv.10", "Niv.25", "Niv.50", "BAGA", "SEQ 7", "LACO", "FORMA MAX", "EM FORMA",
    "BRILHANTE", "DEX 10", "DEX 50", "DEX 100", "DEX 151", "Niv.100", "AMIGO", "FESTA", "JOGAR", "FLORESTA", "PRAIA", "CAVERNA", "SOLAR", "CAMPO" },
  { "Lv.10", "Lv.25", "Lv.50", "きのみ", "7にち", "なかよし", "さいしゅう", "げんき",
    "いろちがい", "ずかん 10", "ずかん 50", "ずかん 100", "ずかん 151", "Lv.100", "ともだち", "パーティ", "あそぼう", "もり", "うみべ", "どうくつ", "やかた", "のはら" },
  { "Lv.10", "Lv.25", "Lv.50", "열매", "연속 7일", "유대", "최고 컨디션", "건강함",
    "이로치", "도감 10", "도감 50", "도감 100", "도감 151", "Lv.100", "친구", "파티", "놀이", "숲", "해변", "동굴", "저택", "들판" },
};
static const char *const MED_LBL[LANG_COUNT][MED_COUNT] = {
  { "Nv10", "Nv25", "Nv50", "BAYA", "7DIAS", "VINC", "TOPE", "SANO",
    "VARIO", "DEX10", "DEX50", "DEX100", "DEX151", "Nv100", "AMIGO", "FIESTA", "JUEGO", "BOSQUE", "PLAYA", "CUEVA", "MANS", "CAMPO" },
  { "Lv10", "Lv25", "Lv50", "BERRY", "7DAYS", "BOND", "TOP", "FIT",
    "SHINY", "DEX10", "DEX50", "DEX100", "DEX151", "Lv100", "BUDDY", "PARTY", "PLAY", "FOREST", "BEACH", "CAVE", "MANSN", "FIELD" },
  { "Niv10", "Niv25", "Niv50", "BAIE", "7JRS", "LIEN", "MAX", "FORME",
    "CHROMA", "DEX10", "DEX50", "DEX100", "DEX151", "Niv100", "COPAIN", "FETE", "JEU", "FORET", "PLAGE", "GROTTE", "MANOIR", "CHAMP" },
  { "Lv10", "Lv25", "Lv50", "BEERE", "7TAGE", "BND", "END", "FIT",
    "SHINY", "DEX10", "DEX50", "DEX100", "DEX151", "Lv100", "KUMPEL", "PARTY", "SPIEL", "WALD", "STRAND", "HOEHLE", "SPUK", "WIESE" },
  { "Lv10", "Lv25", "Lv50", "BACCA", "7GG", "LEG", "MAX", "FIT",
    "CROMA", "DEX10", "DEX50", "DEX100", "DEX151", "Lv100", "AMICO", "FESTA", "GIOCO", "FOREST", "SPIAGG", "GROTTA", "VILLA", "PRATO" },
  { "Niv10", "Niv25", "Niv50", "BAGA", "7DIAS", "LACO", "MAX", "FIT",
    "BRILHO", "DEX10", "DEX50", "DEX100", "DEX151", "Niv100", "AMIGO", "FESTA", "JOGO", "FLORES", "PRAIA", "CAVERN", "SOLAR", "CAMPO" },
  { "Lv10", "Lv25", "Lv50", "きのみ", "7にち", "なかよし", "しんか", "げんき",
    "いろ", "ずかん10", "ずかん50", "ずかん100", "ずかん151", "Lv100", "とも", "パーティ", "あそび", "もり", "うみべ", "どうくつ", "やかた", "のはら" },
  { "Lv10", "Lv25", "Lv50", "열매", "7일", "유대", "최고", "건강",
    "이로치", "도감10", "도감50", "도감100", "도감151", "Lv100", "친구", "파티", "놀이", "숲", "해변", "동굴", "저택", "들판" },
};
static const char *const MED_DSC[LANG_COUNT][MED_COUNT] = {
  { "NIVEL 10", "NIVEL 25", "NIVEL 50", "BAYA HALLADA",
    "RACHA 7 DIAS", "VINCULO MAX", "FORMA FINAL", "EN FORMA",
    "VARIOCOLOR", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "NIVEL 100", "PRIMER AMIGO", "FIESTA DE 5", "JUGAR JUNTOS", "BOSQUE 100%", "PLAYA 100%", "CUEVA 100%", "MANSION 100%", "CAMPO 100%" },
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "BERRY FOUND",
    "7 DAY STREAK", "MAX BOND", "FINAL FORM", "IN SHAPE",
    "SHINY FOUND", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "LEVEL 100", "FIRST BUDDY", "PARTY OF 5", "PLAY TOGETHER", "FOREST 100%", "BEACH 100%", "CAVE 100%", "MANSION 100%", "FIELD 100%" },
  { "NIVEAU 10", "NIVEAU 25", "NIVEAU 50", "BAIE TROUVEE",
    "SERIE 7 JOURS", "LIEN MAX", "FORME FINALE", "EN FORME",
    "CHROMATIQUE", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "NIVEAU 100", "1ER COPAIN", "FETE A 5", "JOUER ENSEMBLE", "FORET 100%", "PLAGE 100%", "GROTTE 100%", "MANOIR 100%", "CHAMP 100%" },
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "BEERE GEFUNDEN",
    "7 TAGE SERIE", "MAX BINDUNG", "ENDFORM", "FIT",
    "SCHILLERND", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "LEVEL 100", "1. KUMPEL", "PARTY ZU 5", "MITSPIELEN", "WALD 100%", "STRAND 100%", "HOEHLE 100%", "SPUKHAUS 100%", "WIESE 100%" },
  { "LIVELLO 10", "LIVELLO 25", "LIVELLO 50", "BACCA TROVATA",
    "SERIE 7 GIORNI", "LEGAME MAX", "FORMA FINALE", "IN FORMA",
    "CROMATICO", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "LIVELLO 100", "PRIMO AMICO", "FESTA DA 5", "GIOCO INSIEME", "FORESTA 100%", "SPIAGGIA 100%", "GROTTA 100%", "VILLA 100%", "PRATO 100%" },
  { "NIVEL 10", "NIVEL 25", "NIVEL 50", "BAGA ACHADA",
    "SEQ 7 DIAS", "LACO MAX", "FORMA FINAL", "EM FORMA",
    "BRILHANTE", "POKEDEX 10", "POKEDEX 50", "POKEDEX 100", "POKEDEX 151", "NIVEL 100", "AMIGO NOVO", "FESTA DE 5", "JOGAR JUNTOS", "FLORESTA 100%", "PRAIA 100%", "CAVERNA 100%", "SOLAR 100%", "CAMPO 100%" },
  { "レベル 10", "レベル 25", "レベル 50", "きのみ はっけん",
    "7にち れんぞく", "なかよし MAX", "さいしゅうしんか", "げんきいっぱい",
    "いろちがい", "ずかん 10", "ずかん 50", "ずかん 100", "ずかん 151", "レベル 100", "はじめての ともだち", "5ひき パーティ", "いっしょに あそぶ", "もり 100%", "うみべ 100%", "どうくつ 100%", "やかた 100%", "のはら 100%" },
  { "레벨 10", "레벨 25", "레벨 50", "열매 발견",
    "연속 7일", "유대감 최대", "최종 진화", "건강함",
    "이로치", "도감 10", "도감 50", "도감 100", "도감 151", "레벨 100", "첫 친구", "5마리 파티", "함께 놀기", "숲 100%", "해변 100%", "동굴 100%", "저택 100%", "들판 100%" },
};

const char *T(StrId id) { return STRINGS[gLang][id]; }
const char *medalName(int i)  { return MED_NAME[gLang][i]; }
const char *medalLabel(int i) { return MED_LBL[gLang][i]; }
const char *medalDesc(int i)  { return MED_DSC[gLang][i]; }

void loadLang() {
  Preferences p;
  p.begin("tamapoke", true);  // read-only
  uint8_t v = p.getUChar("lang", LANG_DEFAULT);
  p.end();
  gLang = (v < LANG_COUNT) ? (Lang)v : LANG_DEFAULT;
}

void setLang(Lang l) {
  if (l >= LANG_COUNT) return;
  gLang = l;
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang", (uint8_t)l);
  p.end();
}
