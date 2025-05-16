import * as BishopDark from "./assets/Chess_bdt60.png";
import * as BishopLight from "./assets/Chess_blt60.png";
import * as KingDark from "./assets/Chess_kdt60.png";
import * as KingLight from "./assets/Chess_klt60.png";
import * as KnightDark from "./assets/Chess_ndt60.png";
import * as KnightLight from "./assets/Chess_nlt60.png";
import * as PawnDark from "./assets/Chess_pdt60.png";
import * as PawnLight from "./assets/Chess_plt60.png";
import * as QueenDark from "./assets/Chess_qdt60.png";
import * as QueenLight from "./assets/Chess_qlt60.png";
import * as RookDark from "./assets/Chess_rdt60.png";
import * as RookLight from "./assets/Chess_rlt60.png";

import * as ChessMove from "./assets/move.ogg";
import * as ChessCapture from "./assets/capture.ogg";
import * as ChessCastle from "./assets/castle.ogg";

export const ASSETS: { [key: string]: string } = {
    "Chess_bdt60.png": BishopDark,
    "Chess_blt60.png": BishopLight,
    "Chess_kdt60.png": KingDark,
    "Chess_klt60.png": KingLight,
    "Chess_ndt60.png": KnightDark,
    "Chess_nlt60.png": KnightLight,
    "Chess_pdt60.png": PawnDark,
    "Chess_plt60.png": PawnLight,
    "Chess_qdt60.png": QueenDark,
    "Chess_qlt60.png": QueenLight,
    "Chess_rdt60.png": RookDark,
    "Chess_rlt60.png": RookLight,
    "move.ogg": ChessMove,
    "capture.ogg": ChessCapture,
    "castle.ogg": ChessCastle,
};

export const IMAGE_WIDTH = 60;
