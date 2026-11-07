# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Prepare a user-selected disc with the existing exporter, entirely on the device."""
import json
from pathlib import Path

from disc import Disc, LEVEL_COUNT, SECTOR
from extract import export_port_level, export_port_title, export_port_flight, levels_stamp_name
from occlusion import write_stamp
from port import levels_format


def prepare(data_folder, progress):
    data = Path(data_folder)
    iso = data / "iso_data/rac1/disc.iso"
    progress("Vérification de votre disque…")
    with Disc(iso) as disc:
        if disc.version.key != "ntsc":
            raise ValueError("Cette version d’OpenRAC prend en charge le disque américain original (SCUS_971.99).")
        boot = disc.executable()
        (iso.parent / "SCUS_971.99").write_bytes(boot)
        (iso.parent / "buildinfo.json").write_text(json.dumps({
            "serial": "SCUS_971.99", "version": "rac1/ntsc", "elf_sha1": disc.version.executable_sha1,
            "files": len(disc.files()), "image": "disc.iso",
        }) + "\n")
        for entry in disc.files():
            if entry["name"] == "SYSTEM.CNF":
                (iso.parent / "SYSTEM.CNF").write_bytes(disc.read(entry["lba"] * SECTOR, entry["bytes"]))
        survey = disc.survey()
    levels = data / "port/ntsc"
    levels.mkdir(parents=True)
    for number in range(LEVEL_COUNT):
        progress(f"Préparation des niveaux : {number + 1} / 21")
        export_port_level(iso, survey["levels"][number], levels, 0)
    progress("Préparation du menu : 20 / 21")
    export_port_title(iso, survey, levels, 0)
    progress("Préparation du vol : 21 / 21")
    export_port_flight(iso, survey, levels, 0)
    write_stamp(levels)
    (levels / levels_stamp_name("rac1")).write_text(json.dumps({"format": levels_format("rac1")}) + "\n")
    progress("Votre jeu est prêt.")
