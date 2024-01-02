#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>

#define BAR 10

using namespace std;

class Song {
public:
    string name;
    sf::Time duration;

    Song(const string& n, const sf::Time& d) : name(n), duration(d) {}
};

class Playlist {
private:
    vector<Song> songs;
    string playlistName;

public:
    Playlist(const string& name) : playlistName(name) {}

    inline void addSong(const Song& song) {
        songs.push_back(song);
    }

    void playPlaylist() {
        for (const auto& song : songs) {
            cout << "Now playing: " << song.name << endl;
        }
    }

    const string& getPlaylistName() const {
        return playlistName;
    }
    const vector<Song>& getSongs() const {
        return songs;
    }

    sf::Time getTotalDuration() const {
        sf::Time totalDuration = sf::Time::Zero;
        for (const auto& song : songs) {
            totalDuration += song.duration;
        }
        return totalDuration;
    }

    // Operator overloading to compare playlists based on total duration
    bool operator<(const Playlist& other) const {
        return getTotalDuration() < other.getTotalDuration();
    }

    void saveToFile(ofstream& file) const {
        file << playlistName << endl;
        for (const auto& song : songs) {
            file << song.name << endl;
        }
    }

    void loadFromFile(ifstream& file) {
        string songName;
        while (getline(file, songName)) {
            if (songName.empty()) {
                // Empty line indicates the end of a playlist
                break;
            }
            songs.push_back(Song(songName, sf::Time::Zero));
        }
    }
};

class PlaylistManager {
protected:
    queue<string> q;

public:
    virtual void playNextSong() = 0;
    void addToQueue(const string& song);
    virtual void createPlaylist(const string& playlistName) = 0;
    virtual void addToPlaylist(const string& playlistName, const string& song) = 0;
    void playPlaylist(const string& playlistName);
};

class MusicPlayer : public PlaylistManager {
private:
    sf::Music music;
    vector<Playlist> playlists;
    float volume;

    void adjustVolume(float delta) {
        volume += delta;
        volume = std::max(0.f, std::min(100.f, volume));  // Clamp between 0 and 100
        music.setVolume(volume);
        cout << "Volume: " << volume << endl;
    }

    static bool fileExists(const string& name) {
        string basePath = "D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\songs\\";
        string fullPath = basePath + name;

        ifstream file(fullPath.c_str());

        if (file.is_open()) {
            file.close();
            return true;
        }
        return false;
    }

    void loadQueueFromFile() {
        ifstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\queue.txt");
        if (file.is_open()) {
            string song;
            while (getline(file, song)) {
                q.push(song);
            }
            file.close();
        }
        else {
            cerr << "Failed to open queue file." << endl;
        }
    }

    void saveQueueToFile() {
        ofstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\queue.txt");
        if (file.is_open()) {
            queue<string> temp = q;
            while (!temp.empty()) {
                file << temp.front() << endl;
                temp.pop();
            }
            file.close();
        }
        else {
            cerr << "Failed to open queue file for writing." << endl;
        }
    }
    void savePlaylistsToFile() {
        ofstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\playlists.txt");
        if (file.is_open()) {
            for (const auto& playlist : playlists) {
                playlist.saveToFile(file);
                file << endl;  // Add an empty line between playlists
            }
            file.close();
        }
        else {
            cerr << "Failed to open playlists file for writing." << endl;
        }
    }

    void loadPlaylistsFromFile() {
        ifstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\playlists.txt");
        if (file.is_open()) {
            string line;
            Playlist playlist("Default");  // Default playlist to store songs not in any playlist

            while (getline(file, line)) {
                if (line.empty()) {
                    // Empty line indicates the end of a playlist
                    if (!playlist.getSongs().empty()) {
                        // Add the playlist only if it's not empty
                        playlists.push_back(playlist);
                    }
                    playlist = Playlist("");  // Reset for the next playlist
                }
                else {
                    // Non-empty line represents either playlist name or a song name
                    if (playlist.getSongs().empty()) {
                        // If the playlist has no songs yet, set the playlist name
                        playlist = Playlist(line);
                    }
                    else {
                        // Otherwise, it's a song name, so add it to the playlist
                        playlist.addSong(Song(line, sf::Time::Zero));
                    }
                }
            }

            // Add the last playlist if it's not empty
            if (!playlist.getSongs().empty()) {
                playlists.push_back(playlist);
            }

            file.close();
        }
        else {
            cerr << "Failed to open playlists file." << endl;
        }
    }


    // Function to get the duration of a song
    sf::Time getSongDuration(const string& song) const {
        string basePath = "D:\\Songs\\";
        string fullPath = basePath + song;

        sf::Music tempMusic;
        if (!tempMusic.openFromFile(fullPath)) {
            cerr << "Failed to open file: " << song << endl;
            return sf::Time::Zero;
        }
        else {
            sf::Time duration = tempMusic.getDuration();
            tempMusic.stop();
            return duration;
        }
    }

public:
    MusicPlayer() {
        loadQueueFromFile();
        loadPlaylistsFromFile();
    }

    const vector<Playlist>& getPlaylists() const {
        return playlists;
    }

    void playMusic() {

        sf::RenderWindow window(sf::VideoMode(400, 200), "Music Player");
        window.setFramerateLimit(60);

        // Play and pause button
        sf::ConvexShape playPauseButton;
        playPauseButton.setPointCount(3);
        playPauseButton.setPoint(0, sf::Vector2f(0, 0));
        playPauseButton.setPoint(1, sf::Vector2f(50, 25));
        playPauseButton.setPoint(2, sf::Vector2f(0, 50));
        playPauseButton.setFillColor(sf::Color::Green);

        // Center the button in the window
        playPauseButton.setPosition((window.getSize().x - playPauseButton.getLocalBounds().width) / 2.f,
            (window.getSize().y - playPauseButton.getLocalBounds().height) / 2.f);

        bool isPlaying = false;

        // Visualizer bars
        const int visualizerBarCount = BAR;
        sf::RectangleShape visualizerBars[visualizerBarCount];

        // Initialize visualizer bars
        const float barWidth = 10.f;
        const float barSpacing = 5.f;
        const float windowHeight = window.getSize().y;

        // Calculate the starting position for the visualizer bars
        float totalWidth = visualizerBarCount * (barWidth + barSpacing) - barSpacing; // Adjusted totalWidth calculation
        float startX = (window.getSize().x - totalWidth) / 2.f;


        // Adjusted height scale for the visualizer bars
        const float heightScale = 0.2; // Adjust this value as needed

        for (int i = 0; i < visualizerBarCount; ++i) {
            // Calculate a random height within the specified scale
            float randomHeight = static_cast<float>(rand() % static_cast<int>(windowHeight * heightScale));

            visualizerBars[i].setSize(sf::Vector2f(barWidth, randomHeight));

            // Calculate the position of each bar within the specified range
            float xPos = startX + i * (barWidth + barSpacing);
            visualizerBars[i].setPosition(xPos, (windowHeight - randomHeight) / 2); // Vertically center the bars
            visualizerBars[i].setFillColor(sf::Color::White);
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                else if (event.type == sf::Event::MouseButtonPressed) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                    // Check if the play/pause button is clicked
                    if (playPauseButton.getGlobalBounds().contains(mousePos)) {
                        if (!isPlaying) {
                            music.play();
                            isPlaying = true;
                        }
                        else {
                            music.pause();
                            isPlaying = false;
                        }
                    }
                }
                else if (event.type == sf::Event::KeyPressed) {
                    // Handle keyboard shortcuts
                    switch (event.key.code) {
                    case sf::Keyboard::Space:
                        if (!isPlaying) {
                            music.play();
                            isPlaying = true;
                        }
                        else {
                            music.pause();
                            isPlaying = false;
                        }
                        break;
                    case sf::Keyboard::S:
                        music.stop();
                        isPlaying = false;
                        break;
                    case sf::Keyboard::Up:
                        adjustVolume(10);  // Increase volume
                        break;
                    case sf::Keyboard::Down:
                        adjustVolume(-10);  // Decrease volume
                        break;
                        // Add more keyboard shortcuts as needed
                    case sf::Keyboard::Right:
                        music.setPlayingOffset(music.getPlayingOffset() + sf::seconds(5));
                        break;
                    case sf::Keyboard::Left:
                        music.setPlayingOffset(music.getPlayingOffset() - sf::seconds(5));
                        break;
                    case sf::Keyboard::N:
                        playNextSong();
                        break;
                    }
                    
                }
            }

            // Update visualizer bars based on random heights
            if (isPlaying) {
                for (int i = 0; i < visualizerBarCount; ++i) {
                    float randomHeight = static_cast<float>(rand() % 200);  // Adjust the range as needed
                    visualizerBars[i].setSize(sf::Vector2f(barWidth, randomHeight));
                    visualizerBars[i].setPosition(i * (barWidth + barSpacing), windowHeight - randomHeight);
                }
            }

            window.clear();

            // Draw play/pause button
            window.draw(playPauseButton);

            // Draw visualizer bars
            for (const auto& bar : visualizerBars) {
                window.draw(bar);
            }

            window.display();

            if (!window.isOpen()) {
                music.stop();
            }
        }
    }

    static string findFileWithExtension(const string& name) {
        vector<string> extensions = { ".mp3", ".ogg", ".wav" };
        for (const auto& ext : extensions) {
            string filename = name + ext;
            if (fileExists(filename)) {
                return filename;
            }
        }
        return ""; // Return empty string if no matching file is found
    }

    void addToQueue(const string& song, bool addToFront){
        if (addToFront) {
            // Insert the song at the front of the queue
            queue<string> temp;
            temp.push(song);    

            // Move existing items to temp queue
            while (!q.empty()) {
                temp.push(q.front());
                q.pop();
            }

            // Move items back to the original queue
            while (!temp.empty()) {
                q.push(temp.front());
                temp.pop();
            }
        }
        else {
            // Add the song to the end of the queue
            q.push(song);
        }

        // Save the updated queue to file
        saveQueueToFile();
    }

    void playNextSong() override {
        while (!q.empty()) {
            string song = q.front();
            string basePath = "D:\\Songs\\";
            string fullPath = basePath + song;
            q.pop();
            if (!music.openFromFile(fullPath)) {
                cerr << "Failed to open file: " << song << endl;
            }
            else {
                music.play();
                cout << "Now playing: " << song << endl;

                playMusic();

            }
            saveQueueToFile();
        }

        // Wait for the user to press Enter before continuing
        cout << "Press Enter to continue...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << endl;  // Move to a new line after pressing Enter
        cin.clear();  // Clear any error flags
    }


    void createPlaylist(const string& playlistName) override {
        loadPlaylistsFromFile();

        ifstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\playlists.txt");
        if (file.is_open()) {
            string line;
            bool foundPlaylist = false;

            // Search for the playlist name in the file
            while (getline(file, line)) {
                if (line == playlistName) {
                    foundPlaylist = true;
                    break;
                }
            }

            // If the playlist is found, add songs to the queue and play them
            if (!foundPlaylist) {
                playlists.emplace_back(playlistName);
                cout << "Playlist \"" << playlistName << "\" created." << endl;
                savePlaylistsToFile();
            }
            else {
                cerr << "Playlist with same name already exists." << endl;
            }

            file.close();
        }
        else {
            cerr << "Failed to open playlists file." << endl;
        }
        
    }

    void addToPlaylist(const string& playlistName, const string& song) override {
        // Load playlists from file
        loadPlaylistsFromFile();

        // Check if the playlist already exists
        auto it = find_if(playlists.begin(), playlists.end(), [&](const Playlist& playlist) {
            return playlist.getPlaylistName() == playlistName;
            });

        if (it != playlists.end()) {
            // Get the duration of the song (you may need to modify findFileWithExtension accordingly)
            sf::Time songDuration = getSongDuration(song);
            it->addSong(Song(song, songDuration));
            cout << "Song added to playlist \"" << playlistName << "\"" << endl;
            savePlaylistsToFile();
        }
        else {
            cerr << "Playlist not found." << endl;
        }
    }

    void playPlaylist(const string& playlistName) {
        // Load playlists from file
        loadPlaylistsFromFile();

        ifstream file("D:\\Uni\\3rd Sem\\OOPS\\Music-Player\\Music-Player\\Resources\\playlists.txt");
        if (file.is_open()) {
            string line;
            bool foundPlaylist = false;

            // Search for the playlist name in the file
            while (getline(file, line)) {
                if (line == playlistName) {
                    foundPlaylist = true;
                    break;
                }
            }

            // If the playlist is found, add songs to the queue and play them
            if (foundPlaylist) {
                
                while (getline(file, line)) {
                    if (line.empty()) {
                        // Empty line indicates the end of the playlist
                        break;
                    }

                    // Split the line at the first blank space to get the song name
                    size_t pos = line.find_first_of(" ");
                    string songName = (pos != string::npos) ? line.substr(0, pos) : line;
                    addToQueue(songName,true);
                    saveQueueToFile();
                }

            }
            else {
                cerr << "Playlist not found." << endl;
            }

            file.close();
        }
        else {
            cerr << "Failed to open playlists file." << endl;
        }
    }


    void sortPlaylistsByDuration() {
        sort(playlists.begin(), playlists.end());
    }

};

int main() {
    MusicPlayer player;

    while (true) {
        cout << endl;
        cout << "Music Player:" << endl;
        cout << "-------------" << endl;
        cout << "1. Play song " << endl;
        cout << "2. Play next song" << endl;
        cout << "3. Add song to queue" << endl;
        cout << "4. Create playlist" << endl;
        cout << "5. Add song to playlist" << endl;
        cout << "6. Play playlist" << endl;
        cout << "7. Playlist by Sorted Duration" << endl;
        cout << "8. Exit" << endl;
        cout << "-------------" << endl;

        int choice;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1: {
            string song;
            cout << "Enter the name of the music file (without extension): ";
            cin >> song;
            song = MusicPlayer::findFileWithExtension(song);
            if (!song.empty()) {
                // Add the song to the front of the queue
                player.addToQueue(song, true);
                player.playNextSong();  // Play the specific song immediately
            }
            else {
                cout << "File not found." << endl;
            }
            break;
        }
        case 2:
            player.playNextSong();
            break;
        case 3: {
            string song;
            cout << "Enter the name of the music file (without extension): ";
            cin >> song;
            song = MusicPlayer::findFileWithExtension(song);
            if (!song.empty()) {
                player.addToQueue(song, false);
            }
            else {
                cout << "File not found." << endl;
            }
            break;
        }
        case 4: {
            string playlistName;
            cout << "Enter the name of the playlist: ";
            cin >> playlistName;
            player.createPlaylist(playlistName);
            break;
        }
        case 5: {
            string playlistName, song;
            cout << "Enter the name of the playlist: ";
            cin >> playlistName;
            cout << "Enter the name of the song (without extension): ";
            cin >> song;
            song = MusicPlayer::findFileWithExtension(song);
            if (!song.empty()) {
                player.addToPlaylist(playlistName, song);
            }
            else {
                cout << "File not found." << endl;
            }
            break;
        }
        case 6: {
            string playlistName;
            cout << "Enter the name of the playlist: ";
            cin >> playlistName;
            player.playPlaylist(playlistName);
            player.playNextSong();
            break;
        }
        case 7: {
            // Print playlists sorted by total duration
            player.sortPlaylistsByDuration();
            cout << "Playlists sorted by total duration:" << endl;
            for (const auto& playlist : player.getPlaylists()) {
                cout << playlist.getPlaylistName() << " - Total Duration: "
                    << playlist.getTotalDuration().asSeconds() << "s" << endl;
            }
            break;
        }
        case 8:
            return 0;
        default:
            cout << "Invalid choice. Try again." << endl;
        }
    }
    return 0;
}
