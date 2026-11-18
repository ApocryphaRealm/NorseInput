NorseInput bindings
===================

Every *.ini here binds named actions to controls. A [section] is the module that owns the actions; a later file's line
for the same action replaces the earlier one, so a player's own file (named to load last) rebinds anything.

    [NorseRealtimeNative]
    LightAttack = Pad.RB, Mouse.Left
    PowerAttack = Pad.RT, Key.F
    LockOn      = Pad.R3, Key.Q
    Dodge       = Pad.B:double

Controls: Pad.A B X Y LB RB LT RT L3 R3 Back Start Up Down Left Right; Key.<letter, digit, F1-F12, Space, Shift,
Ctrl, Alt, Tab, Enter, Esc>; Mouse.Left Right Middle X1 X2. Gestures: :hold (held 0.35 s), :double (two taps within
0.3 s); without one a control fires when pressed and when released. Keyboard and mouse only count while the game is in
front. Two actions of one module on the same control are a conflict: the later one loses that control and the module's
log says so (NorseRealtimeNative.log, lines starting INPUT:). Console: nrn.bindings re-reads these files.
