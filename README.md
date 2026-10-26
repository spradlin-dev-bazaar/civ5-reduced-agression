# civ5-reduced-agression
Mod for Civilization V via Bazaar... At higher difficulties (king+) reduces the AI's agression so that the likelihood of being attacked stays the same as it is on Prince.  The AI retains their bonuses, and are still harder to fight, but they're not more likely to attack you.

## What you need

- Civilization V with Brave New World, on Windows.
- Bazaar, the multi-DLL mod loader, with its two mods: Bazaar Multi-DLL Mod Loader and Bazaar - VMC Bootstrap (required). Each release of this plugin says which Bazaar version it requires.

## Installing

1. Install Bazaar's two mods.
2. Download the `.civ5mod` file from this plugin's release on GitHub, and put it in `Documents\My Games\Sid Meier's Civilization 5\MODS`.
3. In the game's Mods menu, turn on Bazaar - VMC Bootstrap (required), Bazaar Multi-DLL Mod Loader and Reduced Aggression (Bazaar).
4. When updating, delete the earlier version's `.civ5mod` file and its folder from MODS first.

## Making a release

Run `python tools/release.py check`, then `build`, then `publish --notes FILE`. Publishing needs the GitHub CLI, `gh`, logged in. The steps around them are in `.claude/skills/release/SKILL.md`.
