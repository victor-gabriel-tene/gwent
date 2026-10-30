// Draw in painter order, using the framework helpers and Lab 3 model matrices.

void Tema1::Shape(const char* mesh, float x, float y, float w, float h,
                  glm::vec3 color, float angle) {
    const glm::mat3 model = group * transform2D::Translate(x,y)
        * transform2D::Rotate(angle) * transform2D::Scale(w,h);
    RenderMesh2D(meshes.at(mesh),model,color);
}
void Tema1::Box(Rect r, glm::vec3 color) {
    Shape("t1_quad",r.x+r.w/2,r.y+r.h/2,r.w,r.h,color);
}
void Tema1::Frame(Rect r, glm::vec3 color, float t) {
    Box(Rect(r.x,r.y,r.w,t),color); Box(Rect(r.x,r.y+r.h-t,r.w,t),color);
    Box(Rect(r.x,r.y,t,r.h),color); Box(Rect(r.x+r.w-t,r.y,t,r.h),color);
}
void Tema1::Line(float x1, float y1, float x2, float y2, float width, glm::vec3 color) {
    Shape("t1_quad",(x1+x2)/2,(y1+y2)/2,
          std::sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)),width,color,std::atan2(y2-y1,x2-x1));
}
float Tema1::TextWidth(const std::string& value, float size) const {
    float width = 0;
    for (unsigned char c : value) {
        const auto glyph = textRenderer->Characters.find(c < 128 ? c : '?');
        if (glyph != textRenderer->Characters.end()) width += glyph->second.Advance >> 6;
    }
    return width * size / 32;
}
void Tema1::Label(const std::string& value, float x, float top, float size, glm::vec3 color) {
    const float sy = glm::length(glm::vec2(group[1])), sx = glm::length(glm::vec2(group[0]));
    if (sx < sy*.85f) return;
    const glm::vec3 p = group*glm::vec3(x,top,1);
    // Geometry uses a bottom-left origin; the supplied text helper uses top-left.
    textRenderer->RenderText(value,p.x,H-p.y,size*sy/32,color);
}
void Tema1::Center(const std::string& value, float x, float top, float size, glm::vec3 color) {
    Label(value,x-TextWidth(value,size)/2,top,size,color);
}
glm::vec3 Tema1::TypeColor(Type t) const {
    static const glm::vec3 colors[] = {
        {.57f,.13f,.10f}, {.17f,.39f,.24f}, {.15f,.32f,.51f},
        {.075f,.075f,.085f}, {.80f,.83f,.81f}, {.76f,.60f,.20f},
        {.43f,.48f,.50f}, {.23f,.36f,.49f}, {.71f,.29f,.10f},
        {.41f,.24f,.46f}, {.22f,.46f,.35f}, {.45f,.32f,.18f}
    };
    return colors[static_cast<int>(t)];
}

void Tema1::BoardBackground() {
    Box(Rect(0,0,W,H),glm::vec3(.13f,.105f,.08f));
    Box(Rect(258,26,1124,848),glm::vec3(.065f,.045f,.029f));
    Frame(Rect(259,27,1122,846),brass,2);
    for (int owner = 0; owner < 2; ++owner) for (int i = 0; i < 3; ++i) {
        const Lane lane = static_cast<Lane>(i);
        const Rect r = LaneRect(owner,lane);
        Box(r,owner == 0 ? glm::vec3(.27f,.235f,.18f) : glm::vec3(.20f,.185f,.16f));
        Frame(r,glm::vec3(.37f,.28f,.18f),1);
        Box(Rect(r.x,r.y,78,r.h),glm::vec3(.12f,.105f,.085f));
        const float x = r.x+39, y = r.y+43;
        if (lane == Lane::Melee) {
            Line(x-10,y-12,x+10,y+12,3,brass);
            Line(x-11,y-4,x-2,y-13,3,brass);
            Shape("t1_triangle",x+12,y+14,8,10,brass,-PI/4);
        } else if (lane == Lane::Ranged) {
            Line(x-9,y-13,x+12,y,1.5f,brass); Line(x+12,y,x-9,y+13,1.5f,brass);
            Line(x-9,y-13,x-9,y+13,1,brass); Line(x-13,y,x+15,y,2,brass);
            Shape("t1_triangle",x+15,y,7,8,brass,-PI/2);
        } else {
            Box(Rect(x-15,y-10,30,17),brass);
            for (int j = 0; j < 3; ++j) Box(Rect(x-15+j*12,y+7,6,7),brass);
            Shape("t1_circle",x-9,y-14,7,7,brass); Shape("t1_circle",x+9,y-14,7,7,brass);
        }
        Center(LaneName(lane),x,r.y+15,7.5f,muted);
    }
    Line(274,450,1366,450,2,brass);
    Box(Rect(274,34,1092,140),glm::vec3(.10f,.085f,.065f));
    Box(Rect(274,746,1092,130),glm::vec3(.10f,.085f,.065f));
    Line(280,182,1360,182,1,brass); Line(280,734,1360,734,1,brass);
    Center("GWENT",132,868,30,gold); Center("THE CARD TABLE",132,830,10,muted);
    Label("LEFT MOUSE  PICK UP / PLACE",32,43,8,muted);
    Label("BACKSPACE   CANCEL DRAG",32,27,8,muted);
    Center("H  RULES",1485,49,12,ivory);
}

void Tema1::Board() {
    BoardBackground();
    if (dragged >= 0) {
        for (Lane lane : game.LegalLanes(0,dragged)) {
            const Rect r = LaneRect(0,lane);
            const bool inside = r.Contains(mouse);
            Box(Rect(r.x+78,r.y,r.w-78,r.h),inside ? glm::vec3(.39f,.32f,.20f) : glm::vec3(.31f,.27f,.18f));
            Frame(r,inside ? gold : brass,inside ? 2 : 1);
        }
        if (game.cards[dragged].type == Type::Decoy) for (int id : game.players[0].board) {
            if (!game.CanPlay(0,dragged,game.cards[id].lane,id)) continue;
            const Rect r = visuals[id].at;
            Frame(Rect(r.x-3,r.y-3,r.w+6,r.h+6),r.Contains(mouse) ? gold : blue,2);
        }
    }
    Label("ROUND " + Number(game.round) + "  /  TURN " + Number(game.turn),292,724,10,muted);
    Center(detail,905,724,std::min(10.f,690.f/std::max(1.f,TextWidth(detail,1))),muted);
    Center(message,820,196,12,gold);
    const Rect p = PassRect();
    const bool enabled = state == State::Ready && !game.players[0].passed && !help && !inspection;
    Box(p,enabled && p.Contains(mouse) ? brass : glm::vec3(.13f,.10f,.07f));
    Frame(p,enabled ? gold : brass,1);
    Center(game.players[0].passed ? "PASSED" : "PASS ROUND",p.x+p.w/2,p.y+25,11,enabled ? ivory : muted);
    if (Rect(1420,28,130,30).Contains(mouse)) Center("H  RULES",1485,49,12,gold);
}

void Tema1::Status(int owner) {
    const Player& p = game.players[owner];
    const float y = owner == 0 ? 222.f : 458.f;
    Box(Rect(30,y,206,211),dark); Frame(Rect(30,y,206,211),brass,1);
    Center(owner == 0 ? "YOU" : "OPPONENT",133,y+192,19,ivory);
    Line(44,y+164,222,y+164,1,brass);
    Shape("t1_circle",75,y+117,62,62,hitFlash[owner] > 0 ? glm::vec3(.7f,.12f,.08f) : glm::vec3(.18f,.155f,.12f));
    Center(Number(p.hp),75,y+132,27,ivory); Center("HP",75,y+97,10,muted);
    for (int i = 0; i < 30; ++i) {
        const Rect pip(115+(i%10)*10.6f,y+127-(i/10)*14,8,10);
        Box(pip,i < p.hp ? glm::vec3(.70f,.20f,.15f) : glm::vec3(.18f,.14f,.11f));
    }
    Label("SHIELD " + Number(p.shield),44,y+74,10,blue);
    const int columns = 24, rows = std::max(1,(p.shield+columns-1)/columns);
    const float step = std::min(6.f,34.f/rows);
    for (int i = 0; i < p.shield; ++i) Box(Rect(44+(i%columns)*7.5f,y+54-(i/columns)*step,5.5f,std::max(.2f,step*.76f)),blue);
    Center(p.passed ? "PASSED THIS ROUND" : Number(static_cast<int>(p.hand.size()))+" CARDS IN HAND",133,y+20,10,p.passed ? gold : muted);
}

void Tema1::Deck(int owner) {
    const float y = owner == 0 ? 275.f : 530.f;
    const Player& p = game.players[owner];
    Center(owner == 0 ? "YOUR DECK" : "ENEMY DECK",1480,y+160,12,ivory);
    for (int i = 3; i >= 0; --i) {
        const Rect r(1435+i*4.f,y+i*4.f,90,126);
        Box(r,glm::vec3(.13f,.105f,.075f)); Frame(r,brass,1.5f);
    }
    Shape("t1_quad",1480,y+63,32,32,gold,PI/4);
    if (owner == 0) {
        Center(Number(static_cast<int>(p.deck.size()))+" LEFT / "+Number(static_cast<int>(p.discard.size()))+" DISCARDED",1480,y-14,8.5f,gold);
    } else {
        Center(Number(static_cast<int>(p.deck.size()))+" REMAINING",1480,y-14,11,gold);
        Center(Number(static_cast<int>(p.discard.size()))+" DISCARDED",1480,y-36,10,muted);
    }
    if (owner == 0) {
        for (size_t i = 0; i < history.size(); ++i) Center(history[i],1480,182-i*20.f,8,muted);
    }
}

void Tema1::Symbol(Type type, float x, float y, float size, glm::vec3 color) {
    if (type == Type::Damage) Shape("t1_triangle",x,y,size,size,color);
    else if (type == Type::Heal) Shape("t1_quad",x,y,size*.70f,size*.70f,color,PI/4);
    else if (type == Type::Shield) {
        Shape("t1_circle",x,y,size,size,color); Shape("t1_circle",x,y+1,size*.65f,size*.65f,TypeColor(type));
        Shape("t1_quad",x,y,size*.12f,size*.48f,color);
    } else if (type == Type::Spy) {
        Shape("t1_quad",x-size*.18f,y,size*.48f,size*.48f,color,PI/4);
        Shape("t1_quad",x+size*.18f,y,size*.48f,size*.48f,color,PI/4);
        Shape("t1_circle",x,y,size*.26f,size*.26f,TypeColor(type));
    } else if (type == Type::Frost) {
        for (int i = 0; i < 3; ++i) {
            const float a = i*PI/3; const glm::vec2 d(std::cos(a),std::sin(a));
            Line(x-d.x*size/2,y-d.y*size/2,x+d.x*size/2,y+d.y*size/2,size*.065f,color);
        }
        Shape("t1_quad",x,y,size*.26f,size*.26f,color,PI/4);
    } else if (type == Type::Clear) {
        Shape("t1_circle",x,y,size*.55f,size*.55f,color);
        for (int i = 0; i < 8; ++i) { const float a = i*PI/4;
            Shape("t1_triangle",x+std::cos(a)*size*.42f,y+std::sin(a)*size*.42f,size*.14f,size*.20f,color,a-PI/2);
        }
    } else if (type == Type::Fog) {
        for (int i = -1; i <= 1; ++i) Line(x-size*(i == 0 ? .48f : .36f),y+i*size*.25f,x+size*(i == 0 ? .48f : .36f),y+i*size*.25f,size*.10f,color);
    } else if (type == Type::Rain) {
        Shape("t1_circle",x-size*.17f,y+size*.17f,size*.42f,size*.32f,color);
        Shape("t1_circle",x+size*.12f,y+size*.21f,size*.55f,size*.42f,color);
        for (int i = -1; i <= 1; ++i) Line(x+i*size*.25f,y-size*.12f,x+i*size*.25f-size*.12f,y-size*.43f,size*.065f,color);
    } else if (type == Type::Horn) {
        Shape("t1_triangle",x+size*.12f,y,size*.65f,size*.8f,color,PI/2);
        Box(Rect(x-size*.43f,y-size*.10f,size*.40f,size*.20f),color);
        Line(x+size*.48f,y-size*.29f,x+size*.48f,y+size*.29f,size*.08f,color);
    } else if (type == Type::Scorch) {
        Shape("t1_triangle",x,y,size*.75f,size,color);
        Shape("t1_triangle",x-size*.27f,y-size*.13f,size*.40f,size*.58f,color,-.3f);
        Shape("t1_triangle",x+size*.27f,y-size*.13f,size*.40f,size*.58f,color,.3f);
        Shape("t1_triangle",x,y-size*.14f,size*.25f,size*.45f,TypeColor(type));
    } else if (type == Type::Medic) {
        Box(Rect(x-size*.14f,y-size*.48f,size*.28f,size*.96f),color);
        Box(Rect(x-size*.48f,y-size*.14f,size*.96f,size*.28f),color);
    } else if (type == Type::Decoy) {
        Shape("t1_circle",x,y+size*.28f,size*.31f,size*.31f,color);
        Line(x,y+size*.08f,x,y-size*.47f,size*.09f,color);
        Line(x-size*.43f,y-size*.03f,x+size*.43f,y-size*.03f,size*.09f,color);
        Shape("t1_triangle",x,y-size*.20f,size*.62f,size*.50f,color);
    }
}

void Tema1::Card(int id, Rect r, bool face, bool selected, float flip) {
    const glm::mat3 saved = group;
    const tema1::Card& c = game.cards[id];
    group = group * transform2D::Translate(r.x+r.w/2,r.y+r.h/2)
        * transform2D::Scale(r.w/90*flip,r.h/126) * transform2D::Translate(-45,-63);
    CardBody(c,face);
    if (selected) Frame(Rect(-3,-3,96,132),gold,2);
    group = saved;
}

void Tema1::CardBody(const tema1::Card& c, bool face) {
    Box(Rect(0,0,90,126),face ? TypeColor(c.type) : glm::vec3(.17f,.14f,.10f));
    Frame(Rect(0,0,90,126),brass,2);
    if (!face) {
        Shape("t1_quad",45,63,36,36,brass,PI/4);
        Shape("t1_quad",45,63,27,27,dark,PI/4);
        return;
    }
    const glm::vec3 ink = c.type == Type::Frost || c.type == Type::Clear ? dark : ivory;
    const int v = std::max(0,game.EffectiveValue(c.id));
    const bool boosted = v > c.value || c.value > c.printed;
    Shape("t1_quad",49,65,49,49,ink*.65f,Temporary(c.type) ? PI/4 : 0);
    Shape("t1_quad",49,65,45,45,TypeColor(c.type),Temporary(c.type) ? PI/4 : 0);
    Symbol(c.type,49,65,33,ink);
    Shape("t1_circle",19,106,22,22,ivory);
    Center(Number(v),19,114,15,boosted ? glm::vec3(.13f,.40f,.20f) : dark);
    const int rows = v > 6 ? (v+1)/2 : v;
    const float step = std::min(10.f,52.f/std::max(1,rows));
    for (int i = 0; i < v; ++i) {
        const int col = v > 6 ? i/rows : 0, row = v > 6 ? i%rows : i;
        Box(Rect(10+col*8.f,82-row*step,6,std::min(7.f,step*.75f)),boosted ? glm::vec3(.55f,.86f,.47f) : ink);
    }
    const Lane lane = c.zone == Zone::Board ? c.lane : c.printedLane;
    if (lane != Lane::None) {
        const char* shortLane = lane == Lane::Melee ? "M" : lane == Lane::Ranged ? "R" : lane == Lane::Siege ? "S" : "*";
        Shape("t1_quad",71,105,17,17,dark,PI/4); Center(shortLane,71,110,10,ivory);
    }
    Box(Rect(7,8,76,21),ivory);
    const std::string name = Name(c.type);
    Center(name,45,23,std::min(9.f,70.f/std::max(1.f,TextWidth(name,1))),dark);
    Center(c.suppressed ? "SUPPRESSED" : Temporary(c.type) ? "TEMPORARY" : "PERMANENT",47,40,6.5f,ink);
    if (c.suppressed) { Line(34,47,65,80,2,ivory); Line(34,80,65,47,2,ivory); }
    if (boosted) Frame(Rect(4,4,82,118),glm::vec3(.61f,.78f,.38f),1.5f);
}

void Tema1::Tooltip(int id) {
    const tema1::Card& c = game.cards[id];
    if (c.owner == 1 && c.zone == Zone::Hand && !c.known) return;
    const int effective = game.EffectiveValue(id);
    std::string value = "V "+Number(effective);
    if (effective != c.value) value += std::string(" / ")+(Temporary(c.type) ? "LIFETIME " : "BASE ")+Number(c.value);
    if (c.value > c.printed) {
        value += " / PRINTED "+Number(c.printed);
        if (Temporary(c.type) && effective == c.value) value += " / LIFETIME "+Number(c.value);
    }
    const std::string text = std::string(Name(c.type))+" / "+LaneName(c.zone == Zone::Board ? c.lane : c.printedLane)+" / "+value+" / "+Description(c.type);
    Box(Rect(278,704,1084,31),dark); Frame(Rect(278,704,1084,31),brass,1);
    Center(text,820,724,std::min(10.f,1048.f/std::max(1.f,TextWidth(text,1))),ivory);
    Box(Rect(1390,478,188,237),dark); Frame(Rect(1390,478,188,237),brass,1);
    Center("CARD PREVIEW",1484,702,11,gold);
    Card(id,Rect(1422,511,126,176.4f),true);
    Center(value,1484,496,std::min(9.f,174.f/std::max(1.f,TextWidth(value,1))),muted);
}

void Tema1::LeaderShape(Leader leader, float x, float y, float size, glm::vec3 color) {
    if (leader == Leader::FoltestClear) {
        Shape("t1_circle",x,y,size,size,color);
    } else if (leader == Leader::Emhyr) {
        Shape("t1_quad",x,y,size*.73f,size*.73f,color,PI/4);
    } else if (leader == Leader::Eredin) {
        const float radius = size*.53f, height = radius*.8660254f;
        for (int i = 0; i < 6; ++i) {
            const float a = i*PI/3;
            Shape("t1_triangle",x+std::cos(a)*height/2,y+std::sin(a)*height/2,
                  radius,height,color,a+PI/2);
        }
    } else if (leader == Leader::Francesca) {
        Shape("t1_triangle",x,y,size*1.04f,size,color);
    } else if (leader == Leader::FoltestSiege) {
        const float radius = size*.57f, inner = radius*.44f;
        const float baseWidth = 2*inner*std::sin(PI/5), baseAxis = inner*std::cos(PI/5);
        for (int i = 0; i < 5; ++i) {
            const float a = PI/2+i*2*PI/5;
            const float center = (radius+baseAxis)/2;
            Shape("t1_triangle",x+std::cos(a)*center,y+std::sin(a)*center,
                  baseWidth,radius-baseAxis,color,a-PI/2);
            Shape("t1_triangle",x+std::cos(a)*baseAxis/2,y+std::sin(a)*baseAxis/2,
                  baseWidth,baseAxis,color,a+PI/2);
        }
    }
}

void Tema1::LeaderPanel(int owner) {
    const Player& p = game.players[owner];
    const Rect r = LeaderRect(owner);
    const bool usable = !p.leaderUsed && !p.passed;
    const bool hover = r.Contains(mouse);
    Box(r,dark); Frame(r,hover && owner == 0 && usable ? gold : brass,hover ? 2 : 1);
    LeaderShape(p.leader,r.x+39,r.y+43,49,p.leaderUsed ? glm::vec3(.42f) : usable ? gold : muted);
    const std::string name = LeaderName(p.leader);
    Center(name,r.x+r.w/2,r.y+r.h-10,std::min(10.f,(r.w-12)/std::max(1.f,TextWidth(name,1))),ivory);
    Label(p.leaderUsed ? "USED" : p.passed ? "PASSED" : "READY",r.x+78,r.y+46,10,usable ? gold : muted);
    Label(owner == 0 && usable ? "CLICK TO USE" : "LEADER",r.x+78,r.y+25,7.5f,muted);
    for (int i = 0; i < 2; ++i) {
        const float x = r.x+r.w/2-15+i*30;
        Shape("t1_quad",x,r.y-14,14,19,i < p.crystals ? blue : glm::vec3(.17f,.16f,.14f),PI/4);
        Shape("t1_quad",x-2,r.y-11,5,8,i < p.crystals ? glm::vec3(.65f,.85f,.90f) : brass*.4f,PI/4);
    }
    if (hover && !help && !inspection && state == State::Ready) {
        const std::string text = LeaderDescription(p.leader);
        Box(Rect(278,704,1084,31),dark); Frame(Rect(278,704,1084,31),brass,1);
        Center(text,820,724,std::min(10.f,1048.f/std::max(1.f,TextWidth(text,1))),ivory);
    }
}

void Tema1::LeaderMenu() {
    Box(Rect(0,0,W,H),dark);
    Center("GWENT",800,827,50,gold);
    Center("CHOOSE YOUR LEADER",800,716,25,ivory);
    Center("One power for the match. Choose the advantage you want at your side.",800,673,13,muted);
    static const glm::vec3 colors[] = {{.73f,.56f,.26f},{.70f,.64f,.41f},{.50f,.65f,.72f},{.46f,.67f,.42f},{.72f,.45f,.25f}};
    for (int i = 0; i < static_cast<int>(Leader::Count); ++i) {
        const Leader leader = static_cast<Leader>(i); const Rect r = LeaderChoiceRect(i);
        const bool chosen = selectedLeader == i, hover = r.Contains(mouse);
        Box(r,chosen ? glm::vec3(.22f,.19f,.13f) : glm::vec3(.13f,.115f,.09f));
        Frame(r,chosen || hover ? gold : brass,chosen ? 3 : 1);
        if (chosen) LeaderShape(leader,r.x+r.w/2,r.y+211,105*1.15f,blue);
        LeaderShape(leader,r.x+r.w/2,r.y+211,105,chosen ? gold : colors[i]);
        const std::string name = LeaderName(leader);
        Center(name,r.x+r.w/2,r.y+132,std::min(16.f,(r.w-28)/std::max(1.f,TextWidth(name,1))),ivory);
        Line(r.x+24,r.y+108,r.x+r.w-24,r.y+108,1,brass);
        std::istringstream words(LeaderDescription(leader)); std::string word, line; float top = r.y+88;
        while (words >> word) {
            const std::string next = line.empty() ? word : line+" "+word;
            if (!line.empty() && TextWidth(next,10) > r.w-32) { Center(line,r.x+r.w/2,top,10,muted); top -= 17; line = word; }
            else line = next;
        }
        if (!line.empty()) Center(line,r.x+r.w/2,top,10,muted);
        if (chosen) Center("SELECTED",r.x+r.w/2,r.y+22,9,gold);
    }
    Center(selectedLeader >= 0 ? "CLICK YOUR SELECTED LEADER AGAIN TO BEGIN" : "CLICK A LEADER TO SELECT",800,242,17,selectedLeader >= 0 ? gold : ivory);
    Center("First to take both enemy crystals wins. Your leader faces a random opponent.",800,151,11,muted);
}

void Tema1::RulesOverlay() {
    Box(Rect(0,0,W,H),dark);
    Frame(Rect(190,115,1220,670),brass,2);
    Center("THE RULES OF THE TABLE",800,751,29,gold);
    Center("PLAY A CARD, USE YOUR LEADER, OR PASS THE ROUND",800,705,13,muted);
    for (int i = 0; i < 12; ++i) {
        const Type type = static_cast<Type>(i); const int col = i/6, row = i%6;
        const float x = 219+col*595.f, y = 657-row*61.f;
        Symbol(type,x+15,y-10,24,type == Type::Spy ? ivory : TypeColor(type));
        Label(Name(type),x+40,y+1,12,gold);
        std::istringstream words(Description(type)); std::string word, line; float top = y-18;
        while (words >> word) {
            const std::string next = line.empty() ? word : line+" "+word;
            if (!line.empty() && TextWidth(next,9) > 505) { Label(line,x+40,top,9,ivory); top -= 14; line = word; }
            else line = next;
        }
        if (!line.empty()) Label(line,x+40,top,9,ivory);
    }
    Line(220,279,1380,279,1,brass);
    Center("ORDER: LEADER > SCORCH > MEDIC / DECOY > SPY > CLEAR > WEATHER",800,259,11,gold);
    Center("HORN > SHIELD > DAMAGE > HEAL",800,240,11,gold);
    Center("Recurring board effects activate each turn. Temporary cards lose 1 V. Zero V removes a card.",800,215,10.5f,ivory);
    Center("Empty hands, both passed or out of actions: compare HP; shield does not break ties.",800,194,10.5f,ivory);
    Center("Lose a round: lose a crystal. Ties cost both one. New round: 30 HP, no shield; draw one card.",800,173,10.5f,ivory);
    Center("Hands and decks carry over. Each leader power can be used once per match. H OR CLICK TO RETURN.",800,146,10,muted);
}

void Tema1::Render() {
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glDisable(GL_BLEND);
    group = glm::mat3(1);
    if (state == State::SelectLeader) {
        LeaderMenu();
    } else {
        Board(); Status(0); Status(1); Deck(0); Deck(1);
        for (int owner = 0; owner < 2; ++owner) {
            for (int id : game.players[owner].board) {
                bool face = true; float flip = 1;
                if (id == enemyCard && (state == State::EnemyPlace || state == State::Reveal)) {
                    face = false;
                    if (state == State::Reveal) {
                        const float angle = PI*std::min(1.f,clock/.35f);
                        flip = std::max(.015f,std::abs(std::cos(angle))); face = angle >= PI/2;
                    }
                }
                Card(id,visuals[id].at,face,id == hovered,flip);
            }
            for (int id : game.players[owner].hand)
                if (id != dragged) Card(id,visuals[id].at,owner == 0 || game.cards[id].known,id == hovered);
        }
        for (const auto& entry : visuals) if (entry.second.dying && entry.second.vanish < 1) {
            const Visual& v = entry.second; const float scale = 1-v.vanish;
            const Rect r(v.at.x+v.at.w*(1-scale)/2,v.at.y+v.at.h*(1-scale)/2,v.at.w*scale,v.at.h*scale);
            if (scale > .02f) Card(entry.first,r,true);
        }
        LeaderPanel(0); LeaderPanel(1);
        if (hovered >= 0 && dragged < 0) Tooltip(hovered);
        if (dragged >= 0) Card(dragged,visuals[dragged].at,true,true);
        if (state == State::RoundOver || state == State::Over) {
            const bool match = state == State::Over;
            Box(Rect(0,0,W,H),dark);
            Frame(Rect(470,284,660,350),brass,2);
            Center(match ? "MATCH COMPLETE" : "ROUND COMPLETE",800,558,15,muted);
            Center(message,800,520,std::min(39.f,590.f/std::max(1.f,TextWidth(message,1))),gold);
            Center("YOU  " + Number(game.players[0].hp)+" HP   :   "+Number(game.players[1].hp)+" HP  OPPONENT",800,461,20,ivory);
            Center("CRYSTALS   "+Number(game.players[0].crystals)+" : "+Number(game.players[1].crystals),800,421,14,blue);
            Center(detail,800,389,std::min(12.f,600.f/std::max(1.f,TextWidth(detail,1))),muted);
            const Rect button(650,316,300,55);
            Box(button,button.Contains(mouse) ? brass : glm::vec3(.22f,.16f,.09f)); Frame(button,gold,1);
            Center(match ? "PLAY AGAIN  [R]" : "NEXT ROUND",800,353,19,ivory);
        }
        if (inspection) {
            Box(Rect(0,0,W,H),dark);
            Frame(Rect(280,195,1040,510),brass,2);
            Center("THE OPPONENT'S REVEALED CARDS",800,659,25,gold);
            Center("These cards remain visible in their hand.",800,612,12,muted);
            const float step = 156, width = revealed.empty() ? 0 : (revealed.size()-1)*step+126;
            for (size_t i = 0; i < revealed.size(); ++i) Card(revealed[i],Rect(800-width/2+i*step,349,126,176.4f),true);
            const Rect button(690,225,220,48);
            Box(button,button.Contains(mouse) ? brass : glm::vec3(.22f,.16f,.09f)); Frame(button,gold,1);
            Center("CONTINUE",800,258,16,ivory);
        }
    }
    if (help) RulesOverlay();
}
