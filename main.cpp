#include <SFML/Graphics.hpp>
#include <time.h>
#include<iostream>
#include <SFML/Audio.hpp>
#include <fstream>  
#include<cmath>
using namespace sf;
using namespace std;


struct ScoreEntry {
    int score;
    int time;
};

const int MAX_ENTRIES = 5;
const int MAX_ENEMIES = 10; //maximum number of enemies
const int SPEED_INCREASE_INTERVAL = 20; //seconds between speed increases
const int ENEMY_INCREASE_INTERVAL = 20; 
const int PATTERN_SWITCH_TIME = 30; //time when patterns activate
const float CIRCLE_RADIUS = 50.0f;  //radius for circular pattern
const float ZIGZAG_AMPLITUDE = 3.0f; //zigzag pattern amplitude
static int lastSpeedIncreaseTime = 0;
static int lastEnemyIncreaseTime = 0;
static bool patternsActivated = false;
int tilesCapturedThisMove = 0;
int bonusCounter = 0;
int powerUpCount = 0;
bool powerUpActive = false;
Clock powerUpClock;
int currentBonusThreshold = 10;
int currentMultiplier = 2;
float powerUpDuration = 3.0f;

const int MAX_POWERUPS = 10;
int powerUpMilestones[MAX_POWERUPS] = {50, 70, 100, 130, 160, 190, 220, 250, 280, 310};
int nextMilestoneIndex = 0;

const int M = 25;
const int N = 40;

int grid[M][N] = {0};
int ts = 18; //tile size
int enemyCount = 4;
bool continuousMode = false;
int score = 0;
Clock gameClock;      //for elapsed time
bool scoreSaved = false; //so we don’t save multiple times


bool enemiesFrozen = false;
float enemiesFreezeEnd = 0.f;
const int TRAIL_P1 = 2;    
const int TRAIL_P2 = 3;    
bool twoPlayerMode = false;

struct Player {
    int x,y,dx,dy;         //position and direction
    bool alive;            //false when eliminated
    bool building;         //true while laying trail tiles
    int  score;            //personal score
    int  powerUps;         //stock of power-ups
    float freezeEnd;       //gameClock time when this player is frozen
};
Player p1, p2;             

struct Enemy {
    int x, y;
    float dx, dy;
    float patternTimer;
    int patternType; 
    float angle;     
    float baseY;    
    Enemy() : x(300), y(300), dx(0), dy(0), patternTimer(0), patternType(0), angle(0), baseY(300) {
        dx = 4 - rand() % 8;
        dy = 4 - rand() % 8;
    }

    void move() {
        switch (patternType) {
            case 1:  // Circular pattern
                moveCircular();
                break;
            case 2:  // Zigzag pattern
                moveZigzag();
                break;
            default: // Linear movement
                x += dx;
                if (grid[y/ts][x/ts] == 1) { dx = -dx; x += dx; }
                y += dy;
                if (grid[y/ts][x/ts] == 1) { dy = -dy; y += dy; }
        }
    }
    void moveCircular() {
    angle += 0.05f;
    float newX = x + cos(angle) * 3.0f;
    float newY = y + sin(angle) * 3.0f;

    // Check for wall collision
    if (newX > ts && newX < (N-1)*ts && newY > ts && newY < (M-1)*ts) {
        if (grid[int(newY/ts)][int(newX/ts)] == 1) {
            dx = -dx; // Reverse direction
            dy = -dy;
            return; 
        }
    }

  
    if (newX > ts && newX < (N-1)*ts) x = newX;
    if (newY > ts && newY < (M-1)*ts) y = newY;

    // Bounce off screen edges
    if (x <= ts || x >= (N-1)*ts) dx = -dx;
    if (y <= ts || y >= (M-1)*ts) dy = -dy;
}

    // Zigzag movement pattern
    void moveZigzag() {
    patternTimer += 0.10f;
    float newY = baseY + sin(patternTimer) * 30.0f;
    float newX = x + dx;
    if (newX > ts && newX < (N-1)*ts && newY > ts && newY < (M-1)*ts) {
        if (grid[int(newY/ts)][int(newX/ts)] == 1) {
            dx = -dx;
            newX = x + dx;
            return;
        }
    }
    if (newY > ts && newY < (M-1)*ts) y = newY;
    if (newX > ts && newX < (N-1)*ts) x = newX;
    if (x <= ts || x >= (N-1)*ts) {
        dx = -dx;
        x += dx;
    }
}
};


void updateBonusThreshold() {
    if (bonusCounter >= 5) {
        currentMultiplier = 4;
        currentBonusThreshold = 5;
    }
    else if (bonusCounter >= 3) {
        currentBonusThreshold = 5;
         currentMultiplier = 2;
    }
    else {
        currentMultiplier = 2;
        currentBonusThreshold = 10;
    }
}
void checkPowerUpEarned() {
    while (nextMilestoneIndex < MAX_POWERUPS && score >= powerUpMilestones[nextMilestoneIndex]) {
        powerUpCount++;
        nextMilestoneIndex++;
    }
}

void activatePowerUp() {
    if (powerUpCount > 0 && !powerUpActive) {
        powerUpCount--;
        powerUpActive = true;
        enemiesFrozen = true;
        powerUpClock.restart();  // Start the timer
        cout << "Power-up activated! Enemies frozen for 3 seconds." << endl;
    }
}
void updatePowerUpStatus() {
   
    if (powerUpActive && powerUpClock.getElapsedTime().asSeconds() >= powerUpDuration) {
        powerUpActive = false;
        enemiesFrozen = false;
       
    }
}




void activateEnemyPatterns(Enemy enemies[], int enemyCount, int gameTime) {    
    if (!patternsActivated && gameTime >= PATTERN_SWITCH_TIME) {
        patternsActivated = true;
        int patternsAssigned = 0;
        int targetPatterns = enemyCount / 2;
       
        for (int i = 0; i < enemyCount && patternsAssigned < targetPatterns; i++) {
            if (rand() % 2 == 0) {  // Randomize pattern assignment
                enemies[i].patternType = 1; // Circular
                enemies[i].angle = 0;
            } else {
                enemies[i].patternType = 2; // Zigzag
                enemies[i].baseY = enemies[i].y;
                enemies[i].patternTimer = 0;
            }
            patternsAssigned++;
        }
        }
        }


void increaseEnemySpeed(Enemy enemies[], int enemyCount, int gameTime)
{
    
    if (!patternsActivated) return;

    if (gameTime >= lastSpeedIncreaseTime + SPEED_INCREASE_INTERVAL)
    {
        lastSpeedIncreaseTime = gameTime;

        for (int i = 0; i < enemyCount; ++i)
        {
            enemies[i].dx *= 1.25f;    
            enemies[i].dy *= 1.25f;

            if (enemies[i].dx >  8) enemies[i].dx =  8;
            if (enemies[i].dx < -8) enemies[i].dx = -8;
            if (enemies[i].dy >  8) enemies[i].dy =  8;
            if (enemies[i].dy < -8) enemies[i].dy = -8;
        }
    }
}

// void increaseEnemySpeed(Enemy enemies[], int enemyCount, int gameTime) {
//     if (gameTime >= lastSpeedIncreaseTime + SPEED_INCREASE_INTERVAL) {
//         lastSpeedIncreaseTime = gameTime;
//         float speedMultiplier = min(1.0f + (gameTime / 20.0f), 1.25f); // Cap at 3x speed
       
//         for (int i = 0; i < enemyCount; i++) {
//             enemies[i].dx += enemies[i].dx/2.0f;
//             enemies[i].dy += enemies[i].dy/2.0f;
//             // enemies[i].dx = (enemies[i].dx > 0) ? 4.0f * speedMultiplier : -4.0f * speedMultiplier;
//             // enemies[i].dy = (enemies[i].dy > 0) ? 4.0f * speedMultiplier : -4.0f * speedMultiplier;
//         }
//     }
// }

// Function to increase enemies (only in continuous mode)
void increaseEnemies(Enemy enemies[], int& enemyCount, int gameTime) {
    if (!continuousMode) return; // Only in continuous mode
    static int lastIncreaseTime = 0;
    if (gameTime >= lastIncreaseTime + 20 && enemyCount + 2 <= MAX_ENEMIES) {
        lastIncreaseTime = gameTime;
        enemies[enemyCount] = Enemy();
        enemies[enemyCount + 1] = Enemy();
        enemyCount += 2;
        }
}








void drop(int y,int x)
{
    if (y < 0 || y >= M || x < 0 || x >= N) return;

    if (grid[y][x] != 0) return;

    grid[y][x] = -1;

    drop(y-1,x);
    drop(y+1,x);
    drop(y,x-1);
    drop(y,x+1);
}
//RESET FOR EVERY NEW GAMEPLAY
void resetGameState() {
    score = 0;
    tilesCapturedThisMove = 0;
    bonusCounter = 0;
    powerUpCount = 0;
    powerUpActive = false;
    currentBonusThreshold = 10;
    currentMultiplier = 2;
    nextMilestoneIndex = 0;
    scoreSaved = false;
    lastSpeedIncreaseTime = 0;
    lastEnemyIncreaseTime = 0;
    patternsActivated = false;
    //reset player positions and stats
    p1.alive = true;
    p2.alive = true;

    if(twoPlayerMode){
        p1 = { N-11,0,0,0,true,false,0,0,0.f };
        p2 = { 10,0,0,0,true,false,0,0,0.f };
    }else{
        p1 = { N-11,0,0,0,true,false,0,0,0.f  };   //reuse p1 for solo
    }


    gameClock.restart();
    if (continuousMode) {
        enemyCount = 2;
    }
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            grid[i][j] = (i == 0 || j == 0 || i == M - 1 || j == N - 1) ? 1 : 0;
           
            int newMilestones[MAX_POWERUPS] = {50, 70, 100, 130, 160, 190, 220, 250, 280, 310};
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerUpMilestones[i] = newMilestones[i];
    }
}

//MENU
int Menu(RenderWindow& window) {
    Font font;
    if (!font.loadFromFile("images/Golden Age Shad.ttf")) {
        cout << "Error loading font!" << endl;
        return -1;
    }

    Texture backgroundTexture;
    Sprite backgroundSprite;
    if (!backgroundTexture.loadFromFile("images/bg3.jpg")) {
        cout << "Error loading background!" << endl;
    } else {
        backgroundSprite.setTexture(backgroundTexture);
        backgroundSprite.setScale(
            window.getSize().x / (float)backgroundTexture.getSize().x,
            window.getSize().y / (float)backgroundTexture.getSize().y
        );
    }

    Text options[4] = {
        Text("1-Player Game", font, 35),
        Text("2-Player Game", font, 35),
        Text("View Scoreboard",  font, 35),
        Text("Exit",          font, 35)
    };


    float centerX = window.getSize().x / 2.0f;
    float startY = window.getSize().y / 2.0f - 100;

    for (int i = 0; i < 4; ++i) {
        FloatRect textRect = options[i].getLocalBounds();
        options[i].setOrigin(textRect.width / 2, textRect.height / 2);
        options[i].setPosition(centerX, startY + i * 50);
        options[i].setFillColor(Color::White);
    }

    int selectedItem = 0;
    options[selectedItem].setFillColor(Color::White);

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Up) {
                    selectedItem = (selectedItem + 3) % 4;
                } else if (event.key.code == Keyboard::Down) {
                    selectedItem = (selectedItem + 1) % 4;
                }

                for (int i = 0; i < 4; ++i)
                    options[i].setFillColor(i == selectedItem ? Color::Cyan : Color::White);

                if (event.key.code == Keyboard::Enter)
                    return selectedItem;
            }
        }

        window.clear();
        window.draw(backgroundSprite);
        for (int i = 0; i < 4; ++i)
            window.draw(options[i]);
        window.display();
    }

    return 2;
}
//SELECT DIFFICULTY
int SelectDifficulty(RenderWindow& window) {
    Font font;
    if (!font.loadFromFile("images/Golden Age Shad.ttf")) {
        cout << "Error loading font!" << endl;
        return -1;
    }
     //bg image
    Texture bgTexture;
    if (!bgTexture.loadFromFile("images/bg3.jpg")) {  // or any name
        cout << "Error loading difficulty background!" << endl;
    }
    Sprite bgSprite;
    bgSprite.setTexture(bgTexture);
    bgSprite.setScale(
        window.getSize().x / (float)bgTexture.getSize().x,
        window.getSize().y / (float)bgTexture.getSize().y
    );

    string levels[] = {
        "Easy (2 enemies)",
        "Medium (4 enemies)",
        "Hard (6 enemies)",
        "Continuous Mode"
    };

    Text options[4];
    float centerX = window.getSize().x / 2.0f;
    float startY = window.getSize().y / 2.0f - 100;

    for (int i = 0; i < 4; ++i) {
        options[i].setFont(font);
        options[i].setString(levels[i]);
        options[i].setCharacterSize(35);
        FloatRect textRect = options[i].getLocalBounds();
        options[i].setOrigin(textRect.width / 2, textRect.height / 2);
        options[i].setPosition(centerX, startY + i * 60);
        options[i].setFillColor(Color::White);
    }

    int selectedItem = 0;
    options[selectedItem].setFillColor(Color::Cyan);

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {

            if (event.type == Event::Closed)
            {
                window.close();
                return -1;                      // treat as back option
            }

            if (event.type == Event::KeyPressed)
            {
                //esc for back to menu
                if (event.key.code == Keyboard::Escape)
                    return -1;

                if (event.key.code == Keyboard::Up)
                    selectedItem = (selectedItem + 3) % 4;
                else if (event.key.code == Keyboard::Down)
                    selectedItem = (selectedItem + 1) % 4;

                for (int i = 0; i < 4; ++i)
                    options[i].setFillColor(i == selectedItem ? Color::Cyan: Color::White);

                if (event.key.code == Keyboard::Enter)
                    return selectedItem;
            }
        }

        window.clear();
        window.draw(bgSprite);
        for (int i = 0; i < 4; ++i)
            window.draw(options[i]);
        window.display();
    }

    return -1;

}

//SCOREBOARD UPDATES(FOR HIGHSCORE)
bool updateScoreboard(int score, int elapsedTime)
{
    ScoreEntry entries[MAX_ENTRIES + 1];
    int count      = 0;
    int bestBefore = -1;                  

    ifstream in("scoreboard.txt");          
    if (in)
    {
        if (in >> entries[0].score >> entries[0].time)  
            bestBefore = entries[0].score;

        count = 1;
        while (count < MAX_ENTRIES && in >> entries[count].score >> entries[count].time)
            ++count;
    }
    in.close();
      if (count == 0)       //empty file
        bestBefore = -1;  
    entries[count].score = score;
    entries[count].time  = elapsedTime;
    ++count;                                

    for (int i = 0; i < count - 1; ++i)
        for (int j = i + 1; j < count; ++j)
            if (entries[j].score > entries[i].score)
            {
                ScoreEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }

    ofstream out("scoreboard.txt");
    for (int i = 0; i < MAX_ENTRIES && i < count; ++i)
        out << entries[i].score << ' ' << entries[i].time << '\n';
    out.close();

    //is this a new high score?
    return score > bestBefore;                //true if we beat the old top 1
}

//DISPLAY SCOREBOARD
int showScoreboard(RenderWindow& window) {
    Font font;
    font.loadFromFile("images/Golden Age Shad.ttf");

    Text title("Top 5 Scores", font, 40);
    title.setPosition(200, 50);

    Text lines[5];
    ifstream infile("./scoreboard.txt");
    int s, t, i = 0;
    while (infile >> s >> t && i < 5) {
        lines[i].setFont(font);
        lines[i].setCharacterSize(30);
        lines[i].setString("Score: " + to_string(s) + "  Time: " + to_string(t) + "s");
        lines[i].setPosition(200, 120 + i * 50);
        i++;
    }
    infile.close();

    while (window.isOpen())
{
    Event e;
    while (window.pollEvent(e))
    {
        if (e.type == Event::Closed)
        {
            window.close();
            return -1;                      //treat as back option
        }

        if (e.type == Event::KeyPressed)
        {
            //esc as back to menu
            if (e.key.code == Keyboard::Escape)
                return -1;
        }
    }

    window.clear(Color::Black);
    window.draw(title);
    for (int j = 0; j < i; ++j) window.draw(lines[j]);
    window.display();
}

return -1;

}

//END MENU
int EndMenu(RenderWindow& window)
{
    const float VIRTUAL_W = N * ts;  
    const float VIRTUAL_H = M * ts;  
    View menuView(FloatRect(0, 0, VIRTUAL_W, VIRTUAL_H));
    window.setView(menuView);              

 
    Font font;
    if (!font.loadFromFile("images/Golden Age Shad.ttf"))
    { cout << "EndMenu: font load error\n"; return 2; }

    Texture bgTex;
    if (!bgTex.loadFromFile("images/bg3.jpg"))
        cout << "EndMenu: background load error\n";

    Sprite bg(bgTex);                        
    bg.setScale(
        VIRTUAL_W / static_cast<float>(bgTex.getSize().x),
        VIRTUAL_H / static_cast<float>(bgTex.getSize().y));

    Text title("GAME OVER", font, 60);
    {   FloatRect r = title.getLocalBounds();
        title.setOrigin(r.left + r.width/2.f, r.top + r.height/2.f);
        title.setPosition(VIRTUAL_W/2.f, 100.f);
    }

    Text opt[3] = { Text("Restart Game", font, 40),
                    Text("Main Menu",    font, 40),
                    Text("Exit",         font, 40) };

    for (int i = 0; i < 3; ++i)
    {
        FloatRect r = opt[i].getLocalBounds();
        opt[i].setOrigin(r.left + r.width/2.f, r.top + r.height/2.f);
        opt[i].setPosition(VIRTUAL_W/2.f, 200.f + i*70.f);
        opt[i].setFillColor(Color::White);
    }
    int sel = 0;
    opt[sel].setFillColor(Color::Cyan);

    while (window.isOpen())
    {
        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed) { window.close(); return 2; }

            if (e.type == Event::KeyPressed)
            {
                if (e.key.code == Keyboard::Up)   sel = (sel + 2) % 3;
                if (e.key.code == Keyboard::Down) sel = (sel + 1) % 3;

                for (int i = 0; i < 3; ++i)
                    opt[i].setFillColor(i == sel ? Color::Cyan : Color::White);

                if (e.key.code == Keyboard::Enter)
                {   window.setView(window.getDefaultView());  
                    return sel;                                
                }
            }
        }

        window.clear();
        window.draw(bg);
        window.draw(title);
        for (auto& t : opt) window.draw(t);
        window.display();
    }

    window.setView(window.getDefaultView());  
    return 2;
}

   void reinitEnemies(Enemy a[], int &enemyCount)
{
    enemyCount = continuousMode ? 2 :          // or 4 / 6 depending on difficulty
                 (enemyCount == 6 ? 6 :
                 (enemyCount == 4 ? 4 : 2));

    for (int i = 0; i < enemyCount; ++i)
        a[i] = Enemy();        // brand-new enemy with fresh dx/dy & patternType
}

//GAME LOOP
void playGame(RenderWindow& window, bool twoplayer) {
    gameClock.restart();    
    window.setFramerateLimit(60);

    Texture t1, t2, t3;
    t1.loadFromFile("images/tiles.png");
    t2.loadFromFile("images/gameover.png");
    t3.loadFromFile("images/enemy.png");

    Sprite sTile(t1), sGameover(t2), sEnemy(t3);
    sGameover.setPosition(100, 100);
    sEnemy.setOrigin(20, 20);

    //  static int lastSpeedIncreaseTime = 0;
    //  static int lastEnemyIncreaseTime = 0;
    Enemy a[MAX_ENEMIES];
    reinitEnemies(a, enemyCount);
    // winner :  1 = P1   2 = P2   0 = tie 
    int winner = -1;
    bool Game=true;
   

    float timer = 0, delay = 0.07;
    Clock clock;
    int moveCount = 0;
    auto clampXY = [&](int& gx, int& gy)
    {
        if (gx < 0)   gx = 0;
        if (gx > N-1) gx = N-1;
        if (gy < 0)   gy = 0;
        if (gy > M-1) gy = M-1;
    };
    Font moveFont, timeFont;
    Font font;
    Text moveText, timeText;
    moveFont.loadFromFile("images/Golden Age Shad.ttf");
    moveText.setFont(moveFont);
    moveText.setCharacterSize(20);
    moveText.setFillColor(Color::White);
    moveText.setPosition(20, 20);
    RectangleShape moveBox(Vector2f(150, 30));
    moveBox.setFillColor(Color(0, 0, 0, 150));
    moveBox.setPosition(moveText.getPosition().x - 10, moveText.getPosition().y);

    timeFont.loadFromFile("images/Golden Age Shad.ttf");
    timeText.setFont(moveFont);
    timeText.setCharacterSize(20);
    timeText.setFillColor(Color::White);
    timeText.setPosition(590, 20);
    RectangleShape timeBox(Vector2f(130, 30));
    timeBox.setFillColor(Color(0, 0, 0, 150));
    timeBox.setPosition(timeText.getPosition().x - 10, timeText.getPosition().y);


    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            if (i == 0 || j == 0 || i == M - 1 || j == N - 1) grid[i][j] = 1;

    while (window.isOpen()) {
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;
        int gameTime = gameClock.getElapsedTime().asSeconds();
       
       
         
    // In the main game loop, after increaseEnemySpeed and increaseEnemies:
    // In the main game loop, after enemy movement updates:


        increaseEnemySpeed(a, enemyCount, gameTime);  // Speed increases (all modes)
        if(continuousMode)
   increaseEnemies(a, enemyCount, gameTime);
   activateEnemyPatterns(a, enemyCount, gameTime);

        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed)
                window.close();
            if (e.type == Event::KeyPressed && e.key.code == Keyboard::Escape) {
                for (int i = 1; i < M - 1; i++)
                    for (int j = 1; j < N - 1; j++)
                        grid[i][j] = 0;
                p1.x= 10; p1.y= 0;
                Game = true;
            }
        }

    if (!twoPlayerMode){
       
        if (Keyboard::isKeyPressed(Keyboard::Left))       { p1.dx = -1; p1.dy = 0; }
        else if (Keyboard::isKeyPressed(Keyboard::Right)) { p1.dx =  1; p1.dy = 0; }
        else if (Keyboard::isKeyPressed(Keyboard::Up))    { p1.dx =  0; p1.dy = -1; }
        else if (Keyboard::isKeyPressed(Keyboard::Down))  { p1.dx =  0; p1.dy =  1; }
    }  

    else                            
    {
            /* P1 : Arrow keys (unchanged) */
        if (Keyboard::isKeyPressed(Keyboard::Left )) { p1.dx = -1; p1.dy =  0; }
        else if (Keyboard::isKeyPressed(Keyboard::Right)) { p1.dx =  1; p1.dy =  0; }
        else if (Keyboard::isKeyPressed(Keyboard::Up   )) { p1.dx =  0; p1.dy = -1; }
        else if (Keyboard::isKeyPressed(Keyboard::Down )) { p1.dx =  0; p1.dy =  1; }

        /* P2 : WASD – **identical logic**, no direction reset */
        if (Keyboard::isKeyPressed(Keyboard::A))      { p2.dx = -1; p2.dy =  0; }
        else if (Keyboard::isKeyPressed(Keyboard::D)) { p2.dx =  1; p2.dy =  0; }
        else if (Keyboard::isKeyPressed(Keyboard::W)) { p2.dx =  0; p2.dy = -1; }
        else if (Keyboard::isKeyPressed(Keyboard::S)) { p2.dx =  0; p2.dy =  1; }

    }


        if (Keyboard::isKeyPressed(Keyboard::Space)) {
            activatePowerUp();
        }
        updatePowerUpStatus();
        if (!powerUpActive) {
            for (int i = 0; i < enemyCount; i++) {
                a[i].move();
            }
        }

       



if (timer > delay)
{
    auto stepPlayer = [&](Player& pl, int mark)
    {
        if (!pl.alive) return;       

        pl.x += pl.dx;  pl.y += pl.dy; // move 1 tile
        clampXY(pl.x, pl.y);           // stay inside board

        // hit any trail?
        if (grid[pl.y][pl.x] == TRAIL_P1 ||
            grid[pl.y][pl.x] == TRAIL_P2)
            pl.alive = false;

        // empty tile → lay trail
        if (grid[pl.y][pl.x] == 0)
        {
            grid[pl.y][pl.x] = mark;    // 2 or 3
            pl.score++;
            tilesCapturedThisMove++;
            int pointsToAdd = 1;
            moveCount++;

            if (tilesCapturedThisMove > currentBonusThreshold)
            {
                pointsToAdd = currentMultiplier;
            }
            score += pointsToAdd;
            pl.building = true;
            updateBonusThreshold();
            checkPowerUpEarned();
        }

        // touched border → finished area
        if (grid[pl.y][pl.x] == 1) {
            pl.building = false;
            if (tilesCapturedThisMove > 10 )
                bonusCounter++;

            tilesCapturedThisMove = 0; 
        }
    };

    if (twoPlayerMode)             // both players
    {
        stepPlayer(p1, TRAIL_P1); // arrow keys
        stepPlayer(p2, TRAIL_P2); // WASD
    }
    else                            
    {
        stepPlayer(p1, TRAIL_P1);
    }

    timer = 0.f;
}


        if (!p1.alive && (!twoPlayerMode || !p2.alive)){
            if (twoPlayerMode)             
            {
                if      (p1.score >  p2.score) winner = 1;
                else if (p2.score >  p1.score) winner = 2;
                else                           winner = 0;   // tie
            }
        Game = false;
        }

        if (powerUpActive) {
            if (powerUpClock.getElapsedTime().asSeconds() >= 3) {
                powerUpActive = false;
            }
        
        } else {
            for (int i = 0; i < enemyCount; i++) {
                a[i].move();
            }
        }
        //if (!Game) continue;

    //         if (timer > delay) {
    //     x += dx;
    //     y += dy;
    //     x = max(0, min(x, N - 1));
    //     y = max(0, min(y, M - 1));

    //     if (grid[y][x] == 2) Game = false;
    //     if (grid[y][x] == 0) {
    //         grid[y][x] = 2;
    //         tilesCapturedThisMove++;
           
    //         // Calculate points based on current streak
    //         int pointsToAdd = 1;
    //         if (tilesCapturedThisMove > currentBonusThreshold) {
    //             pointsToAdd = currentMultiplier;
    //         }
           
    //         score += pointsToAdd;
    //         moveCount++;
           
    //         // Check if we just crossed the threshold
    //         if (tilesCapturedThisMove == currentBonusThreshold + 1) {
    //             bonusCounter++;
    //             updateBonusThreshold();
    //         }
           
    //         checkPowerUpEarned();
    //     }
    //     else {
    //         // Reset only when move direction changes
    //         if (dx != 0 || dy != 0) {
    //             tilesCapturedThisMove = 0;
    //         }
    //     }

    //     timer = 0;
    // }

        // for (int i = 0; i < enemyCount; i++) a[i].move();

        if (grid[p1.y][p1.x] == 1) {
            p1.dx = p1.dy = 0;
            for (int i = 0; i < enemyCount; i++)
                drop(a[i].y / ts, a[i].x / ts);
            for (int i = 0; i < M; i++){
                for (int j = 0; j < N; j++)
                if      (grid[i][j] == -1)            
                grid[i][j] = 0;  
                else if (grid[i][j] == TRAIL_P1)      
                grid[i][j] = 1;  
                else if (grid[i][j] == 0)            
                grid[i][j] = 1;  
            }
        }

        if (twoPlayerMode && grid[p2.y][p2.x] == 1)
        {
            p2.dx = p2.dy = 0;

            
            for (int i = 0; i < enemyCount; ++i)
                drop(a[i].y / ts, a[i].x / ts);

           
            for (int i = 0; i < M; ++i)
                for (int j = 0; j < N; ++j)
                {
                    if      (grid[i][j] == -1)            
                    grid[i][j] = 0;
                    else if (grid[i][j] == TRAIL_P2)      
                    grid[i][j] = 1;
                    else if (grid[i][j] == 0)            
                    grid[i][j] = 1;
                }

        }    


        for (int i = 0; i < enemyCount; ++i)
        {
            int gy = a[i].y / ts, gx = a[i].x / ts;
            if (grid[gy][gx] == TRAIL_P1 || grid[gy][gx] == TRAIL_P2)
                Game = false;
        }


        moveText.setString("Moves: " + to_string(moveCount));
        timeText.setString("Time: " + to_string(gameTime) + "s");

        window.clear();
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++) {
                if (grid[i][j] == 0) continue;
                if(grid[i][j]==1)
            sTile.setTextureRect(IntRect(0 ,0,ts,ts));   //border  filled
        else if(grid[i][j]==TRAIL_P1)          /* 2 */
            sTile.setTextureRect(IntRect(54,0,ts,ts));   
        else if(grid[i][j]==TRAIL_P2)          /* 3 */
            sTile.setTextureRect(IntRect(72,0,ts,ts));  


                sTile.setPosition(j * ts, i * ts);
                window.draw(sTile);
            }

        sTile.setTextureRect(IntRect(36,0,ts,ts));   //head sprite

        if(!twoPlayerMode){
            sTile.setPosition(p1.x*ts, p1.y*ts);           //solo head
            window.draw(sTile);
        }else{
            sTile.setPosition(p1.x*ts, p1.y*ts);     
            window.draw(sTile);
            sTile.setPosition(p2.x*ts, p2.y*ts);     
            window.draw(sTile);
        }

        sEnemy.rotate(10);
        for (int i = 0; i < enemyCount; i++) {
            sEnemy.setPosition(a[i].x, a[i].y);
            window.draw(sEnemy);
        }

   Text bonusText(" ", moveFont, 14); // Smaller font for compact display
bonusText.setPosition(window.getSize().x - 400, 15); 
bonusText.setFillColor(Color::White); // Stand out but not distract
bonusText.setString(
    "Tiles: " + to_string(tilesCapturedThisMove) + "/" + to_string(currentBonusThreshold) +
    "\n\nMultiplier: x" + to_string(currentMultiplier) +
    "\n\nBonuses: " + to_string(bonusCounter) +
   
    "\n\nScore: " + to_string(score)
   
);
        window.draw(bonusText);



  
    if (twoPlayerMode)
    {
       
        Text p1Hud("", moveFont, 15);
        p1Hud.setFillColor(Color::Yellow);        
        p1Hud.setPosition(window.getSize().x - 180.f, 60.f);        
        p1Hud.setString(
            "P1 Score : "    + to_string(p1.score)      +
            "\n\nP1 Power-ups: " + to_string(powerUpCount));
        window.draw(p1Hud);

       
        Text p2Hud("", moveFont, 15);
        p2Hud.setFillColor(Color::Yellow);
        p2Hud.setPosition(15.f, 60.f);
        p2Hud.setString(
            "P2 Score : "    + to_string(p2.score)      +
            "\n\nP2 Power-ups: " + to_string(powerUpCount));
        window.draw(p2Hud);
    }

    if (powerUpActive) {
        Text powerUpText("POWER UP ACTIVE!", font, 30);
        powerUpText.setPosition(window.getSize().x/2 - 150, 20); 
        powerUpText.setFillColor(Color::Green);
        window.draw(powerUpText);
    }


        if (!Game) {
            window.setView(window.getDefaultView());
            window.clear();

            static bool highScoreChecked = false;
            static bool isHigh = false;

            if (!scoreSaved && !highScoreChecked) {
                int totalTime = gameClock.getElapsedTime().asSeconds();
                isHigh = updateScoreboard(score, totalTime);
                scoreSaved = true;
                highScoreChecked = true;
            }

            window.draw(sGameover);
            if (isHigh) {
                Font highFont;
                if (highFont.loadFromFile("images/Golden Age Shad.ttf")) {
                    Text highText("New High Score!", highFont, 40);
                    highText.setFillColor(Color::Yellow);
                    FloatRect textRect = highText.getLocalBounds();
                    highText.setOrigin(textRect.width / 2, textRect.height / 2);
                    highText.setPosition(window.getSize().x / 2.f, 100.f);
                    window.draw(highText);
                }
            }
            if (twoPlayerMode && winner != -1)
            {
                Font winFont;
                winFont.loadFromFile("images/Golden Age Shad.ttf");

                string msg = (winner == 0) ? "IT'S A TIE!"
                                : (winner == 1) ? "PLAYER 1 WINS!"
                                                : "PLAYER 2 WINS!";

                Text winText(msg, winFont, 40);
                winText.setFillColor(Color::Yellow);
                FloatRect r = winText.getLocalBounds();
                winText.setOrigin(r.width/2.f, r.height/2.f);
                winText.setPosition(window.getSize().x/2.f, 280.f);
                window.draw(winText);
            }

            window.display();
            sf::sleep(seconds(1.5));

            int endChoice = EndMenu(window);

            // if (endChoice == 0)          // 0 = “Restart”
            // {
            //     return;                  // leave playGame(); caller will start a fresh one
            // }
            // else if (endChoice == 1)     // 1 = “Main Menu”
            // {
            //     return;                  // also leave playGame(); you’ll be back at the menu
            // }
            // else                         // 2 = “Exit”
            // {
            //     window.close();
            //     return;
            // }

    if (endChoice == 0) {
        resetGameState();
        p1.x = 26;
        p1.y = 0;
        p1.dx =
        p1.dy = 0;
        moveCount = 0;
        Game = true;
        highScoreChecked = false;
        isHigh = false;
        reinitEnemies(a, enemyCount);      
        lastSpeedIncreaseTime = 0;
        patternsActivated=false;
        continue;
    }

 else if (endChoice == 1) {
                return;
            } else {
                window.close();
                return;
            }
        }


        window.draw(moveBox);
        window.draw(timeBox);
        window.draw(timeText);
        window.draw(moveText);
        window.display();
    }
}

//MAIN
int main() {
   srand(time(0));
    RenderWindow window(VideoMode(N * ts, M * ts), "Xonix Game!");

    Music bgMusic;
    if (!bgMusic.openFromFile("images/Music3.ogg")) {
        cout << "Error loading background music!" << endl;
    } else {
        bgMusic.setLoop(true);
        bgMusic.setVolume(50);
        bgMusic.play();
    }

    while (window.isOpen()) {
        int choice = Menu(window);
        if (choice == 0) {      
            twoPlayerMode = false;
            int difficulty = SelectDifficulty(window);
            if (difficulty != -1) { 
                resetGameState();
                if (difficulty == 0) enemyCount = 2;
                else if (difficulty == 1) 
                enemyCount = 4;
                else if (difficulty == 2)
                enemyCount = 6;
                else if (difficulty == 3) {
                    enemyCount = 2;
                    continuousMode = true;
                }
                playGame(window, false);
            }
        } else if (choice == 1) { 
            twoPlayerMode = true;
            int difficulty = SelectDifficulty(window);
             if (difficulty != -1) { 
                resetGameState();
                if (difficulty == 0) enemyCount = 2;
                else if (difficulty == 1) enemyCount = 4;
                else if (difficulty == 2) enemyCount = 6;
                else if (difficulty == 3) {
                    enemyCount = 2;
                    continuousMode = true;
                }
                playGame(window, true);
            }
        } else if (choice == 2) {
            int back = showScoreboard(window);
            if (back == -1) continue;
        } else if (choice == 3) {
            break;
        }
    }

    return 0;
}


