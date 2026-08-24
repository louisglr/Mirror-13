# MIRROR — by Louis Gabriel

Musikalsk vokal-harmonizer. Manual + MIDI modes. AUTO-mode og den indbyggede
synth er fjernet efter feedback for at holde fokus på lydkvalitet i de to
tilstande, der reelt bruges.

## Vigtigt forbehold om lydæstetik

Målet er en smooth, organisk, bred, drømmeagtig, "dyr"-lydende harmonizer -
IKKE en klon af nogen specifik kommerciel optagelse eller proprietært
værktøj (fx Bon Ivers "715 - CREEKS"/Messina-system). Jeg bygger original
DSP der arbejder hen imod den æstetik I beskriver, med almindeligt kendte
teknikker (granulær pitch-shift, mikro-timing, formant-tilt, let diffusion).
Det bliver aldrig en bit-for-bit reproduktion af noget specifikt optaget
materiale.

## Denne omgang: kvalitetsfokus

**ADVANCED-panelet er fjernet helt** (i stedet for at fejlfinde et skjult
panel jeg ikke selv kan se/klikke på i mit tekst-baserede miljø). Fine Tune,
Tone, Saturation og Micro Delay pr. stemme er nu faste, forsigtige
standardværdier i lyd-motoren i stedet for knapper - færre kontroller, mere
enkelt UI, som efterspurgt. Antal viste decimaler på alle knapper er også
skåret ned (1 decimal i stedet for 3) for et renere udtryk.

**Fjernet:**
- AUTO-mode (og Auto Style-vælgeren) - kun Manual og MIDI tilbage
- Den indbyggede SUPPORT-synth (og AIR/WARMTH) - helt væk, ikke bare skjult

**Rettet (kvalitetsregression fra sidste omgang):**
- CHARACTER-makroen lagde tidligere automatisk ekstra Tone/Saturation/
  Formant oveni, selv ved sin standardværdi (0.25) - det gjorde lyden
  mudret uden at være bedt om det. CHARACTER's standardværdi er nu 0
  (tilføjer intet, medmindre du aktivt skruer op).
- Standard-værdierne for per-stemme TONE og SATURATION er gjort markant
  mere afdæmpede, og selve kurverne er blødere (mindre aggressiv
  lavpasfiltrering, mindre saturation-drive).
- **MICRO DELAY var en "død" parameter** - knappen fandtes, men blev aldrig
  brugt i selve lydbehandlingen. Det er rettet: hver stemme har nu en reel,
  let timing-forskydning (7/13/19/27 ms som udgangspunkt), som er en del af
  det, der skal give "separate sangere"-fornemmelsen frem for "samme stemme
  kopieret".

**Nyt:**
- **AMBIENCE**: en let, subtil allpass-diffusion på harmoni-bussen (ikke på
  dry-signalet) - antydning af rum og bredde uden en tydelig reverb-hale.
  Erstatter den fjernede synth i UI'et. Default er lav (0.2), så det er
  til stede uden at dominere.

## Kendte, fortsat gældende begrænsninger

- Stadig en tidsdomæne-granulær pitch-shifter (3 overlappende korn), ikke en
  ægte spektral/PSOLA-algoritme. Høje, kraftigt opad-transponerede stemmer
  vil altid have lidt mere hørbar processering end moderate intervaller.
- FORMANT er en spektral tilt-EQ, ikke ægte formant-separation.
- MIDI-node-tildeling bruger nu nærmeste-forrige-node-logik (rettet i
  forrige omgang), hvilket markant reducerer store, urolige spring ved
  akkordskift.

## Byg og installer

Samme fremgangsmåde som altid: upload til dit GitHub-repo (erstat alle
Source-filer + CMakeLists.txt), lad Actions bygge, download `Mirror-AU`,
læg `.component` i `~/Library/Audio/Plug-Ins/Components/`, ad-hoc-signér
med `codesign --force --deep --sign -` hvis Logic viser "beskadiget", og
nulstil AU-cachen hvis den ikke dukker op i insert-menuen.
