#include "PgnData.h"

// Incolla qui il file PGN grezzo da studiare
const char CURRENT_STUDY_PGN[] = 
    "[Event \"Paris\"]\r\n"
    "[Site \"Paris FRA\"]\r\n"
    "[Date \"1858.??.??\"]\r\n"
    "[White \"Paul Morphy\"]\r\n"
    "[Black \"Duke Karl / Count Isouard\"]\r\n"
    "[Result \"1-0\"]\r\n"
    "\r\n"
    "1. e4 e5 2. Nf3 d6 3. d4 Bg4 {This is a weak move already. - Fischer} 4. dxe5\r\n"
    "Bxf3 5. Qxf3 dxe5 6. Bc4 Nf6 7. Qb3 Qe7 8. Nc3 c6 9. Bg5 {Black is in zugzwang} 9... b5\r\n"
    "10. Nxb5 cxb5 11. Bxb5+ Nbd7 12. O-O-O Rd8 13. Rxd7 Rxd7 14. Rd1 Qe6 15. Bxd7+\r\n"
    "Nxd7 {Checkmating combination:} 16. Qb8+ $3 Nxb8 17. Rd8# 1-0";