#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include<cmath>
#include<string>

int main() {
    sf::RenderWindow window(sf::VideoMode({1200u, 1000u}), "Horse Training Shooter");

    sf::RectangleShape ship({70.f, 40.f}); // 自機サイズ（仮）
    ship.setPosition({600.f, 900.f});   //初期位置   
    ship.setFillColor(sf::Color::Cyan);

    enum class TrainingAction { Feed, Environment, Love, Discipline, None }; // 育成アクションの列挙

    TrainingAction selectedAction = TrainingAction::None; // 選択中の育成アクション（合成は廃止したため1つのみ保持）

    struct bulletstats { //育成アクションごとに弾のステータスを管理する構造体
        float cooldown; //発射間隔
        float damage; //ダメージ
        bool isRapidFire; //trueなら連射，falseなら単発
    };

    const std::array<bulletstats, 5> bulletstatsarray = {{
        {0.1f, 5.f, true }, //TrainingAction::Feed        (0) 連射しやすい
        {0.45f, 20.f, false}, //TrainingAction::Environment (1) 単発
        {0.15f, 8.f, true }, //TrainingAction::Love        (2) 連射しやすい
        {0.5f, 15.f, false }, //TrainingAction::Discipline  (3) 単発
        {0.f, 0.f, false }, //アクション未選択
    }};

    struct Bullet {
        sf::Vector2f position;
        sf::Vector2f velocity;
        TrainingAction action;
        bool isAlive = true;
        sf::CircleShape shape{10.f}; // 弾の形状（半径10pxの円）
        
        void update(float dt) {
            position += velocity * dt;
            // 等速直線運動
            shape.setPosition(position);
        }

        bool isOffScreen(float screenWidth, float screenHeight)const {
            return position.x < 0 || position.x > screenWidth || position.y < 0 || position.y > screenHeight; // 画面外に出たかどうかを判定し，出ていればOffScreenをtrueにし，表示を消す
        }
    };

    struct Horse { 
        sf::Vector2f position;
        sf::RectangleShape shape{{60.f, 60.f}};
    };

    sf::Vector2f velocity{0.f, 0.f}; // 自機の速度（whileループの外で1回だけ宣言）
    sf::Clock clock;                 // 経過時間を測る時計
    
    sf::Clock bulletClock; // 弾の発射間隔を測る時計
    const float bulletcooldown = 0.1f; //弾のクールタイム

    std::vector<Bullet> bullets; // 弾の配列の管理

    Horse horse; // 馬は1体
    horse.position = {600.f, 150.f};          // 初期位置（x=画面中央付近、y=画面上部）
    horse.shape.setOrigin(horse.shape.getSize() / 2.f); // 中心を原点にする
    horse.shape.setFillColor(sf::Color(139, 69, 19));   // 馬茶色
    horse.shape.setPosition(horse.position);

    int horseDirection = 1; // +1なら右へ、-1なら左へ
    int score = 0;          // 命中回数

    const float HorsefirstSpeed = 100.f; // 基礎移動速度
    const float speedPerPoint  = 2.f;   // スコア1点あたり増える速度

    const float acceleration = 1100.f; // 1秒あたりの加速量（px/s^2）
    const float maxSpeed     = 500.f; // 最高速度（px/s）
    const float friction     = 3.5f;  // 入力がないときの減速の強さ

    bool key1WasPressed = false, key2WasPressed = false, key3WasPressed = false, key4WasPressed = false;
    bool spaceWasPressed = false; // Spaceキーの押下状態
    bool KeyRWasPressed = false; //Rキーの押下状態（リスタートの際のエッジ検出）

    sf::Font font;
    if(!font.openFromFile("assets/fonts/NotoSansJP-VF.ttf")){

    }

    sf::RectangleShape actionBox({30.f, 30.f});
    actionBox.setPosition({20.f, 20.f});
    sf::Text actionlabel(font, "", 18);
    actionlabel.setPosition({20.f, 55.f});
    sf::Text scorelabel(font, "", 24);
    scorelabel.setPosition({20.f, 90.f});
    sf::Text timelabel(font, "", 24);
    timelabel.setPosition({1000.f, 20.f});
    sf::Text resultlabel(font, "", 36);

    sf::Clock gameclock; //ゲームの経過時間
    const float timelimit = 60.f; //制限時間
    bool gameover = false;



    while (window.isOpen()) {
        float dt = clock.restart().asSeconds(); // 前フレームからの経過秒数
        sf::Vector2u windowSize = window.getSize(); // ウィンドウサイズを変数に格納

        float remaining = std::max(0.f, timelimit - gameclock.getElapsedTime().asSeconds());
        if (remaining <= 0.f) {
            gameover = true;
        }

        while (const std::optional event = window.pollEvent()) {  
            if (event->is<sf::Event::Closed>()) window.close();
        }

        if(!gameover){ //つけたしたゲームオーバー後停止処理
        sf::Vector2f input{0.f, 0.f}; // キー入力から向き配列入力
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) input.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) input.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) input.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) input.x += 1.f;

        if (input.length() > 0.f) {
        velocity += input.normalized() * acceleration * dt; // 入力方向に加速
        } else {
        velocity -= velocity * std::min(friction * dt, 1.f); // 摩擦で減速（慣性）
        }

        if (velocity.length() > maxSpeed) {
        velocity = velocity.normalized() * maxSpeed; // 最高速度で頭打ち
        }
        
        ship.move(velocity * dt); // 速度 × 経過時間 だけ位置を動かす

        ship.setOrigin(ship.getSize() / 2.f); // 中心を原点に設定
        sf::Vector2f shipPosition = ship.getPosition(); // 自機の位置を変数に格納
        sf::Vector2f shipSize = ship.getSize(); // 自機のサイズを変数に格納
        
        shipPosition.x = std::clamp(shipPosition.x, shipSize.x, static_cast<float>(windowSize.x) - shipSize.x);
        shipPosition.y = std::clamp(shipPosition.y, shipSize.y, static_cast<float>(windowSize.y) - shipSize.y); // 自機をクランプ

        ship.setPosition(shipPosition); // clamp後位置を更新

        float horseSpeed = HorsefirstSpeed + static_cast<float>(score) * speedPerPoint; // scoreに応じてスピードアップ

        horse.position.x += horseSpeed * static_cast<float>(horseDirection) * dt;

        float horseHalfWidth = horse.shape.getSize().x / 2.f;
        if (horse.position.x < horseHalfWidth) {
            horse.position.x = horseHalfWidth;
            horseDirection = 1; // 左端に達して右へ反転
        } else if (horse.position.x > static_cast<float>(windowSize.x) - horseHalfWidth) {
            horse.position.x = static_cast<float>(windowSize.x) - horseHalfWidth;
            horseDirection = -1; // 右端に達して左へ反転
        }

        horse.shape.setPosition(horse.position);
        }

        auto restart = [&](){
            score = 0;
            gameover = false;
            gameclock.restart();
            selectedAction = TrainingAction::None;
            bullets.clear();

            ship.setPosition({600.f, 900.f});
            velocity = {0.f, 0.f};

            horse.position = {600.f, 150.f};
            horse.shape.setPosition(horse.position);
            horse.shape.setScale({1.f, 1.f});
            horseDirection = 1;

            key1WasPressed = false;
            key2WasPressed = false;
            key3WasPressed = false;
            key4WasPressed = false;
            spaceWasPressed = false;
        };

        if(gameover){
            bool keyRIsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
            if (keyRIsPressed && !KeyRWasPressed) {
                restart();
            }
            KeyRWasPressed = keyRIsPressed;
        }

        auto colorFor = [](TrainingAction action) -> sf::Color {
        switch (action) {
            case TrainingAction::Feed:        return sf::Color(210, 140, 40);  // エサ：小麦色
            case TrainingAction::Environment: return sf::Color::Green;         // 良い環境：緑
            case TrainingAction::Love:        return sf::Color(255, 105, 180); // 愛情：ピンク
            case TrainingAction::Discipline:  return sf::Color::Blue;          // しつけ：青
            default:                          return sf::Color::White;
        }
        };

        auto actionName = [](TrainingAction action) -> std::string {
        switch (action) {
            case TrainingAction::Feed:        return "Feed";
            case TrainingAction::Environment: return "Environment";
            case TrainingAction::Love:        return "Love";
            case TrainingAction::Discipline:  return "Discipline";
            default:                          return "";
        }
        };


        if (selectedAction == TrainingAction::None) {
            actionBox.setFillColor(sf::Color::Transparent);
            actionBox.setOutlineColor(sf::Color(128, 128, 128));
            actionBox.setOutlineThickness(2.f);
            actionlabel.setString("");
        } else {
            actionBox.setFillColor(colorFor(selectedAction));
            actionBox.setOutlineThickness(0.f);
            actionlabel.setString(actionName(selectedAction));
            actionlabel.setFillColor(sf::Color::White);
        }
        scorelabel.setString("SCORE : " + std::to_string(score));
        timelabel.setString("TIME : " + std::to_string(static_cast<int>(remaining)));

        if (gameover){
            resultlabel.setString("Final Score: " + std::to_string(score));

            sf::FloatRect bounds = resultlabel.getLocalBounds();
            resultlabel.setOrigin(bounds.position + bounds.size / 2.f);
            resultlabel.setPosition({static_cast<float>(windowSize.x) / 2.f, static_cast<float>(windowSize.y) / 2.f});
        }

        auto selectAction = [&](TrainingAction a) {
            selectedAction = a; // 最後に選んだ1つだけを保持する
        };

        if(!gameover){
            bool key1IsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1);
            if (key1IsPressed && !key1WasPressed) selectAction(TrainingAction::Feed);
            key1WasPressed = key1IsPressed;

            bool key2IsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2);
            if (key2IsPressed && !key2WasPressed) selectAction(TrainingAction::Environment);
            key2WasPressed = key2IsPressed;

            bool key3IsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3);
            if (key3IsPressed && !key3WasPressed) selectAction(TrainingAction::Love);
            key3WasPressed = key3IsPressed;

            bool key4IsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4);
            if (key4IsPressed && !key4WasPressed) selectAction(TrainingAction::Discipline);
            key4WasPressed = key4IsPressed;

            auto firebullet = [&](const bulletstats& stats, TrainingAction currentAction) {
            Bullet newBullet;
            newBullet.position = ship.getPosition();
            newBullet.velocity = {0.f, -600.f};
            newBullet.action = currentAction;
            newBullet.shape.setPosition(newBullet.position);
            newBullet.shape.setFillColor(colorFor(currentAction));

            bullets.push_back(newBullet);
            bulletClock.restart();
            };

            bool spaceIsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
            if (spaceIsPressed && !spaceWasPressed) {
                if (selectedAction != TrainingAction::None) {
                const bulletstats& stats = bulletstatsarray[static_cast<size_t>(selectedAction)];
                firebullet(stats, selectedAction); //定義したfirebulletを選択中のアクションで呼ぶ

                if (!stats.isRapidFire) {
                    selectedAction = TrainingAction::None; 
                } //isRapidFireがtrueの場合は選択を維持
                }
            };
            spaceWasPressed = spaceIsPressed;
        
            for (auto& bullet : bullets) { //当たり判定
                if (!bullet.isAlive) continue; // 既に消滅済みの弾は無視する

                float dx = bullet.position.x - horse.position.x;
                float dy = bullet.position.y - horse.position.y;

                if (std::abs(dx) < 40.f && std::abs(dy) < 40.f) { // 当たったとみなす
                bullet.isAlive = false; // 弾を消す
                score++;                // 総合能力スコアを1点加算
                }
            };
        }

        for (auto& bullet : bullets) {
            bullet.update(dt); // 弾の位置を更新
        }

        bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
        [&](const Bullet& b) {
            return !b.isAlive || b.isOffScreen(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
        }),
        bullets.end()
        ); // 画面外に出た弾を削除＋馬に当たった弾削除


        window.clear(sf::Color::Black); 
        window.draw(ship);              
        window.draw(horse.shape);
        
        for (const auto& bullet : bullets) {
            window.draw(bullet.shape);
        }

        window.draw(actionBox);
        window.draw(actionlabel);
        window.draw(scorelabel);
        window.draw(timelabel);
        if (gameover) {
            window.draw(resultlabel);
        }
        window.display();
    }
}