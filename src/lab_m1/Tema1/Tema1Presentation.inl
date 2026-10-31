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
void Tema1::Paragraph(const std::string& value, Rect bounds, float size, glm::vec3 color) {
    std::istringstream words(value);
    std::string word, line;
    float top = bounds.y + bounds.h;
    while (words >> word) {
        const std::string next = line.empty() ? word : line + " " + word;
        if (!line.empty() && TextWidth(next,size) > bounds.w) {
            Center(line,bounds.x+bounds.w/2,top,size,color);
            top -= size * 1.5f; line = word;
        } else line = next;
    }
    if (!line.empty()) Center(line,bounds.x+bounds.w/2,top,size,color);
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
    const Rect hand = HandRect(0), enemyHand = HandRect(1);
    Frame(Rect(hand.x-16,hand.y-12,hand.w+32,enemyHand.y+enemyHand.h-hand.y+24),brass,1);
    for (int owner = 0; owner < 2; ++owner) for (int i = 0; i < 3; ++i) {
        const Lane lane = static_cast<Lane>(i);
        const Rect r = LaneRect(owner,lane);
        Box(r,owner == 0 ? glm::vec3(.21f,.185f,.145f) : glm::vec3(.18f,.16f,.13f));
    }
    const Rect lane = LaneRect(0,Lane::Melee);
    Line(lane.x,450,lane.x+lane.w,450,1,brass);
}

void Tema1::Board() {
    BoardBackground();
    if (dragged >= 0) {
        for (Lane lane : game.LegalLanes(0,dragged)) {
            const Rect r = LaneRect(0,lane);
            const bool inside = r.Contains(mouse);
            Box(r,inside ? glm::vec3(.34f,.28f,.18f) : glm::vec3(.27f,.235f,.16f));
            Frame(r,inside ? gold : brass,inside ? 2 : 1);
        }
        if (game.cards[dragged].type == Type::Decoy) for (int id : game.players[0].board) {
            if (!game.CanPlay(0,dragged,game.cards[id].lane,id)) continue;
            const Rect r = visuals[id].at;
            Frame(Rect(r.x-3,r.y-3,r.w+6,r.h+6),r.Contains(mouse) ? gold : blue,2);
        }
    }
    // Draw labels after the drop highlights, inside the reserved lane margin.
    for (int owner = 0; owner < 2; ++owner) for (int i = 0; i < 3; ++i) {
        const Lane lane = static_cast<Lane>(i);
        const Rect r = LaneRect(owner,lane);
        Center(LaneName(lane),r.x+lanePadding/2,r.y+r.h/2+4,9,muted);
    }
    const Rect rules = RulesRect();
    const bool hover = rules.Contains(mouse);
    Box(rules,hover ? glm::vec3(.23f,.19f,.13f) : glm::vec3(.17f,.14f,.10f));
    Frame(rules,hover ? gold : brass,1);
    Center("RULES  [H]",rules.x+rules.w/2,rules.y+rules.h/2+5.5f,11,hover ? gold : ivory);
}

void Tema1::Status(int owner) {
    const Player& p = game.players[owner];
    const float y = LeaderRect(owner).y;
    Center("HP " + Number(p.hp),800,y+42,10,hitFlash[owner] > 0 ? gold : muted);
    for (int i = 0; i < 30; ++i) {
        const Rect pip(681+i*8.f,y+8,6,13);
        Box(pip,i < p.hp ? (hitFlash[owner] > 0 ? gold : glm::vec3(.65f,.24f,.18f)) : glm::vec3(.18f,.14f,.11f));
    }
    Center("SHIELD " + Number(p.shield),1189,y+42,10,blue);
    const int columns = 30, rows = std::max(1,(p.shield+columns-1)/columns);
    const float step = std::min(15.f,22.f/rows), height = std::min(13.f,step*.85f);
    for (int i = 0; i < rows*columns; ++i)
        Box(Rect(1070+(i%columns)*8.f,y+21-(i/columns)*step-height,6,height),
            i < p.shield ? blue : glm::vec3(.12f,.18f,.21f));
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
    if (!face) return;
    const glm::vec3 ink = c.type == Type::Frost || c.type == Type::Clear ? dark : ivory;
    const int v = std::max(0,game.EffectiveValue(c.id));
    const bool boosted = v > c.value || c.value > c.printed;
    Symbol(c.type,49,63,38,ink);
    const int rows = v > 6 ? (v+1)/2 : v;
    const float step = std::min(10.f,52.f/std::max(1,rows));
    for (int i = 0; i < v; ++i) {
        const int col = v > 6 ? i/rows : 0, row = v > 6 ? i%rows : i;
        Box(Rect(10+col*8.f,104-row*step,6,std::min(7.f,step*.75f)),boosted ? glm::vec3(.55f,.86f,.47f) : ink);
    }
    const Lane lane = c.zone == Zone::Board ? c.lane : c.printedLane;
    if (lane != Lane::None) {
        const char* shortLane = lane == Lane::Melee ? "M" : lane == Lane::Ranged ? "R" : lane == Lane::Siege ? "S" : "*";
        Center(shortLane,75,115,12,ink);
    }
    Shape(Temporary(c.type) ? "t1_triangle" : "t1_circle",75,15,8,8,ink);
    if (c.suppressed) { Line(34,47,65,80,2,ivory); Line(34,80,65,47,2,ivory); }
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
    Card(id,Rect(1400,350,144,201.6f),true);
    Center(Name(c.type),1472,332,13,gold);
    Center(Temporary(c.type) ? "TEMPORARY" : "PERMANENT",1472,309,9,muted);
    Paragraph(value,Rect(1380,250,180,39),9,ivory);
    Paragraph(Description(c.type),Rect(1380,100,180,133),10,muted);
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
    const bool usable = !p.leaderUsed;
    const bool hover = r.Contains(mouse);
    const bool active = owner == 0 ? state == State::Ready || state == State::Return || dragged >= 0
        : state == State::EnemyPlace || state == State::Reveal;
    const bool highlight = hover && owner == 0 && usable && state == State::Ready
        && dragged < 0 && !help && !inspection;
    if (highlight) Shape("t1_circle",r.x+24,r.y+23,44,44,glm::vec3(.30f,.25f,.16f));
    LeaderShape(p.leader,r.x+24,r.y+23,highlight ? 34 : 30,p.leaderUsed ? glm::vec3(.42f) : gold);
    const std::string name = LeaderName(p.leader);
    Label(owner == 0 ? "YOU" : "OPPONENT",r.x+50,r.y+42,10,active ? gold : muted);
    Label(name,r.x+50,r.y+21,std::min(10.f,175.f/std::max(1.f,TextWidth(name,1))),ivory);
    for (int i = 0; i < 2; ++i) {
        const float x = r.x+240+i*20;
        Shape("t1_quad",x,r.y+23,10,13,i < p.crystals ? blue : glm::vec3(.23f,.21f,.18f),PI/4);
    }
    if (hover && !help && !inspection && state == State::Ready && dragged < 0 && hovered < 0) {
        LeaderShape(p.leader,1472,480,90,p.leaderUsed ? glm::vec3(.42f) : gold);
        Paragraph(name,Rect(1380,380,180,30),12,gold);
        Center(p.leaderUsed ? "USED" : "ONCE PER MATCH",1472,360,9,muted);
        Paragraph(LeaderDescription(p.leader),Rect(1380,200,180,130),10,ivory);
    }
}

void Tema1::LeaderMenu() {
    Box(Rect(0,0,W,H),dark);
    Center("GWENT",W/2,792,42,gold);
    Center("CHOOSE YOUR LEADER",W/2,716,23,ivory);
    Center(selectedLeader >= 0 ? "Click the selected leader again to begin" : "Click a leader to select",
           W/2,675,12,muted);
    static const glm::vec3 colors[] = {{.73f,.56f,.26f},{.70f,.64f,.41f},{.50f,.65f,.72f},{.46f,.67f,.42f},{.72f,.45f,.25f}};
    for (int i = 0; i < static_cast<int>(Leader::Count); ++i) {
        const Leader leader = static_cast<Leader>(i); const Rect r = LeaderChoiceRect(i);
        const bool chosen = selectedLeader == i, hover = r.Contains(mouse);
        Box(r,chosen ? glm::vec3(.18f,.155f,.115f) : glm::vec3(.13f,.115f,.09f));
        if (chosen || hover) Frame(r,chosen ? gold : brass,1);
        if (chosen) LeaderShape(leader,r.x+r.w/2,r.y+211,105*1.15f,blue);
        LeaderShape(leader,r.x+r.w/2,r.y+211,105,chosen ? gold : colors[i]);
        const std::string name = LeaderName(leader);
        Center(name,r.x+r.w/2,r.y+132,std::min(16.f,(r.w-28)/std::max(1.f,TextWidth(name,1))),ivory);
        Paragraph(LeaderDescription(leader),Rect(r.x+20,r.y+30,r.w-40,58),10,muted);
    }
}

void Tema1::RulesOverlay() {
    Box(Rect(0,0,W,H),dark);
    Frame(Rect(190,75,1220,750),brass,1);
    Center("RULES",W/2,791,28,gold);
    Center("Drag a card to its lane or click your leader",W/2,749,12,muted);
    for (int i = 0; i < 12; ++i) {
        const Type type = static_cast<Type>(i); const int col = i/6, row = i%6;
        const float x = 219+col*595.f, y = 704-row*56.f;
        Symbol(type,x+15,y-10,24,type == Type::Spy ? ivory : TypeColor(type));
        Label(Name(type),x+40,y+1,12,gold);
        std::istringstream words(Description(type)); std::string word, line; float top = y-18;
        while (words >> word) {
            const std::string next = line.empty() ? word : line+" "+word;
            if (!line.empty() && TextWidth(next,10) > 505) { Label(line,x+40,top,10,ivory); top -= 15; line = word; }
            else line = next;
        }
        if (!line.empty()) Label(line,x+40,top,10,ivory);
    }
    Line(225,374,1375,374,1,brass);
    Center("RESOLUTION ORDER",W/2,350,10,muted);
    Center("Leader > Scorch > Medic / Decoy > Spy > Clear > Weather > Horn > Shield > Damage > Heal",W/2,329,11,gold);
    static const char* headings[] = {"CARD VALUES", "ROUND & MATCH", "NEXT ROUND"};
    static const char* notes[3][4] = {
        {"Squares show the current value.", "Circle: permanent. Triangle: temporary.",
         "Temporary cards lose 1 V per activation.", "At 0 V, a card is removed."},
        {"0 HP loses a round and one crystal.", "Both hands empty, or no actions: compare HP.",
         "Ties cost both players one crystal.", "Lose both crystals to lose the match."},
        {"Reset to 30 HP and no shield.", "Keep your hand; draw one card.",
         "Leader powers work once per match.", "Used leaders turn grey."}
    };
    for (int col = 0; col < 3; ++col) {
        const float x = 245+col*400.f;
        Label(headings[col],x,279,12,gold);
        for (int row = 0; row < 4; ++row) Label(notes[col][row],x,252-row*19.f,11,ivory);
    }
    Center("Hover to inspect   /   Backspace cancels a drag   /   H or click to close",W/2,113,11,muted);
}

void Tema1::PopupPanel(const std::string& title) {
    const Rect panel(510,276,580,348);
    Box(Rect(0,0,W,H),dark);
    Box(panel,glm::vec3(.13f,.115f,.09f));
    Frame(panel,brass,1);
    Center(title,W/2,594,28,gold);
    Line(panel.x+32,552,panel.x+panel.w-32,552,1,brass*.55f);
}

void Tema1::PopupButton(const std::string& label) {
    const Rect r = PopupButtonRect();
    const bool hover = r.Contains(mouse);
    Box(r,hover ? glm::vec3(.23f,.19f,.13f) : glm::vec3(.17f,.14f,.10f));
    Frame(r,hover ? gold : brass,1);
    Center(label,r.x+r.w/2,r.y+r.h/2+7.5f,15,hover ? gold : ivory);
}

void Tema1::ResultPopup() {
    const bool match = state == State::Over;
    PopupPanel(message);
    for (int owner = 0; owner < 2; ++owner) {
        const float x = W/2 + (owner == 0 ? -140 : 140);
        const Player& p = game.players[owner];
        Center(owner == 0 ? "YOU" : "OPPONENT",x,518,11,muted);
        if (!match) Center(Number(p.hp),x,482,24,ivory);
        Center(Number(p.crystals),x,match ? 474 : 424,match ? 28 : 24,blue);
    }
    // Each stat label sits between its two values, on the same visual baseline.
    if (!match) Center("HP",W/2,475,11,muted);
    Center("CRYSTALS",W/2,match ? 465.5f : 417.f,11,blue);
    if (match && game.roundResult == Result::Draw)
        Center("FINAL ROUND DRAWN",W/2,408,11,muted);
    PopupButton(match ? "PLAY AGAIN" : "NEXT ROUND");
}

void Tema1::InspectionPopup() {
    PopupPanel("REVEALED CARDS");
    const float step = 156, width = revealed.empty() ? 0 : (revealed.size()-1)*step+126;
    for (size_t i = 0; i < revealed.size(); ++i)
        Card(revealed[i],Rect(W/2-width/2+i*step,364,126,176.4f),true);
    PopupButton("CONTINUE");
}

void Tema1::Render() {
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glDisable(GL_BLEND);
    group = glm::mat3(1);
    if (state == State::SelectLeader) {
        LeaderMenu();
    } else {
        Board(); Status(0); Status(1);
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
        if (state == State::RoundOver || state == State::Over) ResultPopup();
        if (inspection) InspectionPopup();
    }
    if (help) RulesOverlay();
}
