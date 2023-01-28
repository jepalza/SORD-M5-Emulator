Emulator of SORD-M5 computer with cartridge BASIC-I      Joseba Epalza 12-SEP-00
================================================================================

  This emulator, in first version (1.0), is for old computer called SORD-M5,
  an early computer type ZX-81 (with the same memory capacity) and just between
  ZX SPECTRUM and MSX (froms this one take the same graphic chip). Did has a
  rubbers keyboard ultraslim and very sweet. Equipped with 4k RAM for BASIC
  and 16k VRAM for video, and an external connector for expansion cartridges.
  The BASIC not is internal, is external! with BASIC-I cartridge.

  My emulator, now, emulate very few things:

    - The BASIC-I (included in file named: SORDBASI.ROM)
    - The internal ROM (included in file named: SORDINT.ROM)
    - The inclomplete keyboard, not at 100%. See below.
    - The expansion connector: theorically, whatever cartridge is emulated.
    - The complete graphics system, included sprites.

  To emulate in the next version ??? ( !!! Maybe ):

    - Inports and Outports with external world (printers and joysticks)
    - True Save and Load commands for real and virtual management (PC or tape)
    - Improved keyboard. (improved? or performance? or repair? ) ;-)
    - Sounds.
    - True speed emulation?

  By the moment, do has own one SORD-M5 with ONLY ONE BASIC-I cartridge.
  For dumped the cartidge and internal ROM, did make an elctronic circuit for
  connecting printer port (SORD) to parallel port (PC). Very poor and nasty
  solution, but.... it worked!!

  I don't has any ROM CARTIDGE or tapes (games) for try my emulator, and for
  this problem, cannot to prove the emulator to 100%.

  If you are capable to dumpedany more cartridges or tapes,
   please "emiliame" (how the spanish people say).

  Can't sure a future version from this emulator without more roms, and because
  my time is very narrow:  my CAD/CAM work, and my family (three son and wife)
  ocupped all my time.

  ============================================================================

  How the emulator run:


     * With F1 key obtain very sort help.

     * With F2 does can insert cartridge (file of ROM) inthe external connector.
       by default, the SORDBASI.ROM (BASIC-I) is inserted at main.

     * F3 is for save in file DUMP.RAM the content of 4k RAM between 0x7000-0x8000
       plus 256 bytes of CPU Z80 state.

     * With F4 load this content (DUMP.RAM)

     * With F5 key obtain a PCX file for actual screen.

     * The CONTROL key from PC emulate CTRL key from SORD-M5

     * The SHIFT key from PC emulate SHIFT key from SORD-M5

     * The ALT key from PC emulate FN key from SORD-M5

     * The keys not equal to alfabetical or numerical, are very wrong, but
       at least the principal keys are emulated. You must probe it first.

     * Emulates special keys from M5, how CTRL+Q, or CTRL+R, or CTRL+T for
       special graphics combinations (screen modes). Try it!




                           That's all folks!!

                           For criticism or questions:

                           <jepalza@teleline.es>

                           From Sopelana (Vizcaya-Espa¤a).

      P.D.: know that my english is very poor and very, very, very suck,
            but i'm spanish man, and my time of school was about 17 years ago.
            Sorry for this point, chico.



