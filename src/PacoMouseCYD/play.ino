/*    PacoMouseCYD throttle -- F. Cañada 2025-2026 --  https://usuaris.tinet.cat/fmco/
*/


////////////////////////////////////////////////////////////
// ***** STATION RUN - MODEL TRAIN GAME FOR KIDS *****
////////////////////////////////////////////////////////////

void updateStationTime (uint16_t seconds) {
  snprintf(staTimeBuf, ACC_LNG + 1, "%d:%02d", seconds / 60, seconds % 60);
}

void updateStationTarget() {
  uint16_t delta;
  staStations = staLevel + 4;
  if (staStartTime < 10)
    staStartTime = 10;
  delta = staStartTime / 5;                       // time for one station
  staTime = staStartTime + ((staLevel - 1) * delta);
}

void newStationCounters (bool ini) {
  if (ini) {
    staStars = 0;
    staLevel = 1;
    randomSeed(millis());
  }
  staCurrStation = 0;
  updateStationTarget();
}

uint8_t newStation(byte last) {
  uint8_t station;                                // genera numero estacion sin repetir la ultima
  do {
    station = random (0, staMaxStations);
  } while (station == last);
  return (station);
}

void updateTargetStations() {
  snprintf(staStationsBuf, ACC_LNG + 1, "%d", staStations);
}

void updateCountStations() {
  snprintf(staStationsBuf, ACC_LNG + 1, "%d/%d", staCurrStation, staStations);
}

void updateStationLevel() {
  snprintf(staLevelBuf, ADDR_LNG + 1, "%d", staLevel);
}

void updateStationStars() {
  snprintf(staStarsBuf, ADDR_LNG + 1, "%d", staStars);
}

void setNewTarget() {
  uint16_t pos;
  staLastStation = newStation(staLastStation);
  iconData[ICON_STA_TARGET].color = staColors[staLastStation];
  pos = iconData[ICON_STA_TRAIN].x;
  iconData[ICON_STA_TRAIN].x = iconData[ICON_STA_TARGET].x;
  iconData[ICON_STA_TARGET].x = pos;
  iconData[ICON_STA_PIN].x = pos + 8;
}

void clickTargetStation() {
  encoderValue = 0;
  locoData[myLocoData].mySpeed = 0;
  locoOperationSpeed();
  updateSpeedDir();
  staCurrStation++;
  if (staCurrStation == staStations) {
    stopTimer(TMR_STA_RUN);
    staLevel++;
    updateStationLevel();
    newStationCounters(false);
    updateTargetStations();
    updateStationTime(staTime);
    closeWindow(WIN_STA_PLAY);
    openWindow(WIN_STA_STARS);                // well done!
  }
  else {
    updateCountStations();
    setNewTarget();
    newEvent(OBJ_WIN, WIN_STA_PLAY, EVNT_DRAW);
  }
}

uint16_t  staGetTurnoutAdr(uint16_t eeAdr, uint16_t defAdr) {
  uint16_t adr;
  adr = (EEPROM.read(eeAdr) << 8) + EEPROM.read(eeAdr + 1);
  if (adr > 2048)
    adr = defAdr;
  return adr;
}

void updateTurnoutButtons() {
  uint16_t n, icon;
  for (n = 0; n < 4; n++) {
    if (staTurnoutPos[n])
      fncData[FNC_STA_ACC0 + n].idIcon = bitRead(staTurnoutDef, n) ? FNC_TURNRD_OFF : FNC_TURNLD_OFF;
    else
      fncData[FNC_STA_ACC0 + n].idIcon = bitRead(staTurnoutDef, n) ? FNC_TURNRS_OFF : FNC_TURNLS_OFF;
    fncData[FNC_STA_ACC0 + n].colorOn = staTurnoutPos[n] ? COLOR_RED : COLOR_GREEN;
  }
}

////////////////////////////////////////////////////////////
// ***** NEXT TRAIN - CAR CARDS & WAYBILLS GAME *****
////////////////////////////////////////////////////////////

void updateConductorIcon() {
  fncData[FNC_CONDUCTOR].idIcon = playingGame ? FNC_CONDUCTOR_OFF : FNC_BLANK_OFF;
  fncData[FNC_CONDUCTOR].colorOn = colorConductor[currConductor];
  fncData[FNC_CURR_COND].colorOn = colorConductor[currConductor];
}

void updateRound() {
  char buf[NAME_LNG];
  getLabelTxt(LBL_ROUND, buf);
  snprintf(nxtRoundBuf, NAME_LNG + 1, "%s %d", buf, currRound + 1);
}

bool findCard(uint8_t type, uint8_t location, uint16_t id, uint8_t *pos) {
  uint8_t cnt;
  DEBUG_MSG("Finding card %d in %d:%d from %d", type, location, id, *pos)
  for (cnt = *pos; cnt < totalCards; cnt++) {
    if (gameCards[cnt].cardType == type) {
      if (gameCards[cnt].cardLocation == location) {
        if (gameCards[cnt].cardLocationNumber == id) {
          DEBUG_MSG("Found card in %d", cnt)
          *pos = cnt;
          return true;
        }
      }
    }
  }
  return false;
}

uint8_t countCards(uint8_t type, uint8_t location, uint16_t id) {
  uint8_t cnt, pos;
  pos = 0;
  cnt = 0;
  while (findCard(type, location, id, &pos)) {
    cnt++;
    pos++;
  }
  return cnt;
}

void updateOperPoint() {
  uint8_t pos;
  pos = 0;
  if (findCard(CARD_OPER_POINT, CARD_LAYOUT, currOperPoint, &pos)) {
    fncData[FNC_NXT_OPER].idIcon = (gameCards[pos].cardSubType << 1) + FNC_PASS_OFF;
    fncData[FNC_NXT_CAPACITY].num = (gameCards[pos].cardData[0] > 0) ? gameCards[pos].cardData[0] : 99; // capacity
  }
  getOperPointName(nxtOperName, currOperPoint);
  newEvent(OBJ_TXT, TXT_NXT_OPER, EVNT_DRAW);
  newEvent(OBJ_FNC, FNC_NXT_OPER, EVNT_DRAW);
  newEvent(OBJ_FNC, FNC_NXT_CAPACITY, EVNT_DRAW);
}

void updateDestPoint() {
  uint8_t pos;
  pos = 0;
  if (findCard(CARD_OPER_POINT, CARD_LAYOUT, currOperPoint, &pos))
    fncData[FNC_NXT_DEST].idIcon = (gameCards[pos].cardSubType << 1) + FNC_PASS_OFF;
  getOperPointName(nxtOperName, currOperPoint);
  newEvent(OBJ_TXT, TXT_NXT_DEST, EVNT_DRAW);
  newEvent(OBJ_FNC, FNC_NXT_DEST, EVNT_DRAW);
}

void updateOperOrders() {
  uint8_t pos, n;
  pos = 0;
  for (n = 0; n < 4; n++) {
    if (findCard(CARD_ORDER, CARD_LAYOUT, currOperPoint, &pos)) {
      fncData[FNC_NXT_DEST0 + n].idIcon = ((gameCards[pos].cardSubType - CARD_PASSENGER) << 1) + FNC_WPASS_OFF;
      fncData[FNC_NXT_DEST0 + n].num = gameCards[pos].cardData[2];
      txtData[TXT_NXT_DEST0 + n].border = COLOR_WHITE;
      getOperPointName(nxtDestName[n], gameCards[pos].cardData[1]);
      operPointCards[n] = pos;
      pos++;
    }
    else {
      fncData[FNC_NXT_DEST0 + n].idIcon = FNC_BLANK_OFF;
      fncData[FNC_NXT_DEST0 + n].num = 99;
      txtData[TXT_NXT_DEST0 + n].border = COLOR_BACKGROUND;
      nxtDestName[n][0] = '\0';
      operPointCards[n] = NOT_USED;
    }
    newEvent(OBJ_TXT, TXT_NXT_DEST0 + n, EVNT_DRAW);
    newEvent(OBJ_FNC, FNC_NXT_DEST0 + n, EVNT_DRAW);
  }
}

void updateLokOrders(bool show) {
  uint8_t pos, n;
  pos = 0;
  for (n = 0; n < 4; n++) {
    if (findCard(CARD_ORDER, CARD_TRAIN, locoData[myLocoData].myAddr.address, &pos)) {
      fncData[FNC_NXT_WAGON0 + n].idIcon = ((gameCards[pos].cardSubType - CARD_PASSENGER) << 1) + FNC_WPASS_OFF;
      fncData[FNC_NXT_WAGON0 + n].num = gameCards[pos].cardData[2];
      getOperPointName(nxtWagonName[n], gameCards[pos].cardData[1]);
      pos++;
    }
    else {
      fncData[FNC_NXT_WAGON0 + n].idIcon = FNC_BLANK_OFF;
      fncData[FNC_NXT_WAGON0 + n].num = 99;
      nxtWagonName[n][0] = '\0';
    }
    if (show) {
      newEvent(OBJ_TXT, TXT_NXT_WAGON0 + n, EVNT_DRAW);
      newEvent(OBJ_FNC, FNC_NXT_WAGON0 + n, EVNT_DRAW);
    }
  }
}

void shuffleCards() {                             // Baraja las cartas
  uint16_t n, i;
  cardNextTrain tmp;
  randomSeed(millis());
  for (n = 0; n < totalCards; n++) {
    i = random(totalCards);
    tmp = gameCards[n];
    gameCards[n] = gameCards[i];
    gameCards[i] = tmp;
  }
}

void getOperPointName(char *buf, uint8_t opt) {
  uint16_t cnt, pos;
  char chr;
  cnt = 0;
  pos = 0;
  buf[cnt] = '\0';
  chr = operPointNames[pos++];
  while (chr) {
    if (chr == '\n') {
      chr = '\0';
      buf[cnt] = chr;
      if (opt > 0) {
        opt--;
        cnt = 0;
        chr = operPointNames[pos++];
      }
    }
    else {
      buf[cnt++] = chr;
      chr = operPointNames[pos++];
    }
  }
  buf[cnt] = '\0';
}

void takeOrder(uint16_t order) {
  uint8_t pos;
  pos = operPointCards[order];
  if (pos != NOT_USED) {                          // order exist?
    if (countCards(CARD_ORDER, CARD_TRAIN, locoData[myLocoData].myAddr.address) < 4) {      // can carry it?
      gameCards[pos].cardLocation = CARD_TRAIN;
      gameCards[pos].cardLocationNumber = locoData[myLocoData].myAddr.address;
      updateOperOrders();
      updateLokOrders(false);
    }
  }
}

void checkEndOrder() {
  uint8_t pos;
  pos = 0;
  while (findCard(CARD_ORDER, CARD_TRAIN, locoData[myLocoData].myAddr.address, &pos)) {
    if (gameCards[pos].cardData[1] == currOperPoint) {    // destination arrived
      gameCards[pos].cardLocation = CARD_PLAYER;
      gameCards[pos].cardLocationNumber = currConductor;
    }
    pos++;
  }
  updateLokOrders(false);
}


void calcConductorPoints() {
  uint8_t n, pos, points, maxPoints;
  maxPoints = 0;
  for (n = 0; n < 4; n++) {
    //points = countCards(CARD_ORDER, CARD_PLAYER, n);
    points = countCards(CARD_EVENT, CARD_PLAYER, n);      // reward cards unused
    pos = 0;
    while (findCard(CARD_ORDER, CARD_PLAYER, n, &pos)) {  // delivered orders
      points += gameCards[pos].cardData[2];
      pos++;
    }
    snprintf(nxtDestName[n], IP_LNG + 1, "%d", points);
    if (points > maxPoints) {
      maxPoints = points;
      fncData[FNC_NXT_STAR].y = 80 + (n * 40);
    }
  }
}

bool putOrderOnLayout(uint8_t pos) {
  uint8_t idx, cnt, operID, operCapacity, ok;
  ok = false;
  idx = 0;
  operID = gameCards[pos].cardData[0];
  cnt = countCards(CARD_ORDER, CARD_LAYOUT, operID);
  if (findCard(CARD_OPER_POINT, CARD_LAYOUT, operID, &idx)) {
    operCapacity = gameCards[idx].cardData[0];
    if (cnt < operCapacity) {
      gameCards[pos].cardLocation = CARD_LAYOUT;
      gameCards[pos].cardLocationNumber = gameCards[pos].cardData[0];   // from
      ok = true;
    }
    else {
      gameCards[pos].cardLocation = CARD_DISCARD;
    }
  }
  return ok;
}


void newOrderCard(uint8_t pos) {
  fncData[FNC_NXT_POINTS].num = gameCards[pos].cardData[2];       // points
  fncData[FNC_NXT_WAGON].idIcon = ((gameCards[pos].cardSubType - CARD_PASSENGER) << 1) + FNC_WPASS_OFF;
  getOperPointName(nxtDestName[0], gameCards[pos].cardData[0]);   // from
  getOperPointName(nxtDestName[1], gameCards[pos].cardData[1]);   // to
  openWindow(WIN_NXT_ORDER);
  putOrderOnLayout(pos);
}

bool findReward(uint8_t reward) {
  uint8_t pos;
  bool ok;
  ok = false;
  pos = 0;
  while (findCard(CARD_EVENT, CARD_PLAYER, currConductor, &pos)) {
    if (gameCards[pos].cardData[0] == reward) {
      gameCards[pos].cardLocation = CARD_DISCARD;
      ok = true;
    }
    pos++;
  }
  return ok;
}


void newEventCard(uint8_t pos) {
  loadGameEvent(currGame, gameCards[pos].cardData[2]);
  fncData[FNC_NXT_REWARD].colorOn = COLOR_YELLOW;
  fncData[FNC_NXT_REWARD].num = 99;
  fncData[FNC_NXT_REWARD].idIcon = FNC_STAR_OFF;
  if (gameCards[pos].cardSubType == CARD_REWARD) {
    gameCards[pos].cardLocation = CARD_PLAYER;
    gameCards[pos].cardLocationNumber = currConductor;
  }
  else {
    gameCards[pos].cardLocation = CARD_DISCARD;
    if (findReward(gameCards[pos].cardData[0]))
      fncData[FNC_NXT_REWARD].colorOn = COLOR_RED;
    else {
      if (gameCards[pos].cardData[1] > 0) {
        fncData[FNC_NXT_REWARD].idIcon = FNC_LOST_OFF;
        fncData[FNC_NXT_REWARD].num = gameCards[pos].cardData[1];
      }
      else {
        fncData[FNC_NXT_REWARD].idIcon = FNC_BLANK_OFF;
      }
      nextRound[currConductor] += gameCards[pos].cardData[1];
    }
  }
  openWindow(WIN_NXT_EVENT);
}


void getEventCard () {
  uint8_t pos, posOrder;
  pos = 0;
  if (findCard(CARD_ORDER, CARD_PILE, 0, &pos)) {
    posOrder = pos;
    pos = 0;
    if (findCard(CARD_EVENT, CARD_PILE, 0, &pos)) {
      if (posOrder > pos)                         // order & events available, take first
        newEventCard(pos);
      else
        newOrderCard(posOrder);
    }
    else {
      newOrderCard(posOrder);                     // only order available
    }
  }
  else {
    pos = 0;
    if (findCard(CARD_EVENT, CARD_PILE, 0, &pos)) {
      newEventCard(pos);                          // only events available
    }
    else {                                        // no order and no event available -> end game
      playingGame = false;
      updateConductorIcon();
      closeWindow(WIN_PLAY_NEXT);
      openWindow(WIN_NXT_POINTS);
    }
  }
}

void nextConductor() {
  currConductor++;
  if (currConductor == maxConductor) {
    currConductor = 0;
    currRound++;
    updateRound();
  }
}
void nextPlayerTurn (uint16_t id) {
  closeWindow(id);
  nextRound[currConductor]++;
  nextConductor();
  while (nextRound[currConductor] != currRound) {
    nextConductor();
  }
  updateConductorIcon();
}

void initGameNextTrain() {
  uint8_t pos, n;
  closeWindow(WIN_NEXT_TRAIN);
  if (sdDetected && loadGame(currGame)) {
    shuffleCards();
    pos = 0;
    n = 0;
    while (findCard(CARD_ORDER, CARD_PILE, 0, &pos) && (n < 3)) {     // put 3 order cards on layout
      if (putOrderOnLayout(pos))
        n++;
      pos++;
    }
    shuffleCards();
    currConductor = 0;
    currRound = 0;
    nextRound[0] = 0;
    nextRound[1] = 0;
    nextRound[2] = 0;
    nextRound[3] = 0;
    currOperPoint = 0;
    playingGame = true;
    updateConductorIcon();
    openWindow(WIN_PLAY_NEXT);
  }
  else {
    alertWindow(ERR_FILE);
  }
}
