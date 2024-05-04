#include <iostream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <vector>

class DataBase;
class StateSubscription;
class StateSubscription;
class Subscribed;
class Unsubscribed;


class ErrorMessages {
public:
    static void bookExists() { std::cout << "Book already exists\n"; }
    static void userExists() { std::cout << "User already exists\n"; }
    static void userSubscribed() { std::cout << "User already subscribed\n"; }
    static void userUnsubscribed() { std::cout << "User is not subscribed\n"; }
    static void haveNotAccess() { std::cout << "No access\n"; }
};


class SuccessfulMessages {
public:
    static void userNotified(const std::string& username,
                             float priceBook,
                             const std::string& bookTitle)
                                { std::cout << username + " notified about price update for " + bookTitle + " to " + std::to_string(priceBook) + '\n'; }
    static void userReading(const std::string& username,
                            const std::string& bookTitle,
                            const std::string& authorBook)
                                { std::cout << username + " reading " + bookTitle + " by " + authorBook + '\n'; }
    static void userListening(const std::string& username,
                              const std::string& bookTitle,
                              const std::string& authorBook)
                                { std::cout << username + " listening " + bookTitle + " by " + authorBook + '\n'; }
};


typedef struct Book {
    std::string title;
    std::string author;
    float price;
    enum FormatBook{
        Textual,
        Audio,
    };
} Book;


class BookPriceObserver {
public:
    virtual void updatePrice(const std::string& title, float price) = 0;
};


class User {
private:
    std::string _username;
public:
    class StateSubscription* stateSubscription;
    explicit User(const std::string& username);

    void setSubscription(StateSubscription* state) {
        stateSubscription = state;
    }
    void subscribe();
    void unsubscribe();

    std::string getUsername() { return this->_username; }
};


class PremiumUser : public User {
public:
    PremiumUser(const std::string& username) : User(username) {}
};


class StandardUser : public User {
public:
    StandardUser(const std::string& username) : User(username) {}
};


class StateSubscription {
protected:
    User* _subscriber;

public:
    virtual void subscribe(User* user) {
        ErrorMessages::userSubscribed();
    }
    virtual void unsubscribe(User* user) {
        ErrorMessages::userUnsubscribed();
    }
};


class SubscriptionObserver {
public:
    virtual void updateSubscribersAppend(const std::string& username, User* user) = 0;
    virtual void updateSubscribersRemove(const std::string& username, User* user) = 0;
};


class Subscribed : public StateSubscription {
private:
    void notify() {}
public:
    Subscribed() {}
    void unsubscribe(User* user) override;
};


class Unsubscribed : public StateSubscription {
private:
    void notify() {}
public:
    Unsubscribed() {}
    void subscribe(User* user) override {
        notify();
        user->setSubscription(new Subscribed());
        delete this;
    }
};


void Subscribed::unsubscribe(User *user) {
    notify();
    user->setSubscription(new Unsubscribed());
    delete this;
}


void User::subscribe() {
    stateSubscription->subscribe(this);
}

void User::unsubscribe() {
    stateSubscription->unsubscribe(this);
}

User::User(const std::string &username) : _username(username) {
    stateSubscription = new Unsubscribed();
}


class DataBase : public SubscriptionObserver, public BookPriceObserver {
private:
    std::unordered_map<std::string, User*> _premiumUsers;
    std::unordered_map<std::string, User*> _standardUsers;
    std::unordered_map<std::string, Book> _books;
    std::vector<User*> _newsletters;

    void notify(float priceBook, const std::string& titleBook) {
        for (auto & user : _newsletters)
            SuccessfulMessages::userNotified(user->getUsername(), priceBook, titleBook);
    }
public:
    DataBase() {}

    virtual void updateSubscribersAppend(const std::string& username, User* user) override {
        _newsletters.push_back(user);
    }
    virtual void updateSubscribersRemove(const std::string& username, User* user) override {
        for (int i = 0; i < _newsletters.size(); ++i) {
            if (_newsletters[i]->getUsername() == user->getUsername())
                _newsletters.erase(_newsletters.begin() + i);
        }
    }

    virtual void updatePrice(const std::string& title, float price) override {
        _books.find(title)->second.price = price;
        notify(price, title);
    }

    void addBook(const std::string& title, Book book) {
        if (this->_books.find(title) != this->_books.end())
            ErrorMessages::bookExists();
        else
            this->_books.insert({ title, book });
    }
    void addStandardUser(const std::string& username, User* user) {
        if (this->_standardUsers.find(username) != this->_standardUsers.end())
            ErrorMessages::userExists();
        else
            this->_standardUsers.insert({ username, user });
    }
    void addPremiumUser(const std::string& username, User* user) { this->_premiumUsers.insert({ username, user }); }

    User* getUser(const std::string& username) { return _standardUsers.find(username)->second; }
    User* getPremiumUser(const std::string& username) {
        if (_premiumUsers.find(username) != _premiumUsers.end())
            return _premiumUsers.find(username)->second;
        return nullptr;
    }
    Book getBook(const std::string& titleBook) { return _books.find(titleBook)->second; }
};


class UserFactory {
public:
    virtual User* createUser(const std::string& username) = 0;
};


class PremiumUserFactory : public UserFactory {
public:
    User* createUser(const std::string& username) override {
        return new PremiumUser(username);
    }
};


class StandardUserFactory : public UserFactory {
public:
    User* createUser(const std::string& username) override {
        return new StandardUser(username);
    }
};


class OnlineBookStore {
private:
    static OnlineBookStore* store_instance;
    OnlineBookStore() {
        _dataBase = new DataBase();
        _premiumUserFactory = std::make_shared<PremiumUserFactory>();
        _standardUserFactory = std::make_shared<StandardUserFactory>();
    }
    OnlineBookStore( const OnlineBookStore& );
    OnlineBookStore& operator=( OnlineBookStore& );

    DataBase* _dataBase;
    std::shared_ptr<PremiumUserFactory> _premiumUserFactory;
    std::shared_ptr<StandardUserFactory> _standardUserFactory;

    void createBook(std::vector<std::string>& data) {
        std::string title, author;
        float price;
        Book book;

        title = data[0];
        author = data[1];
        price = std::stof(data[2]);
        book = (Book) {
            .title = title,
            .author = author,
            .price = price,
        };

        _dataBase->addBook(title, book);
    }
    void createUser(std::vector<std::string>& data) {
        User* person;
        std::string user_type = data[0];
        std::string username = data[1];

        if (user_type == "Standard")
            person = _standardUserFactory->createUser(username);
        else if (user_type == "Premium") {
            person = _premiumUserFactory->createUser(username);
            _dataBase->addPremiumUser(username, person);
        }

        _dataBase->addStandardUser(username, person);
    }
    void subscribe(std::vector<std::string>& data) {
        std::string username = data[0];

        _dataBase->getUser(username)->subscribe();
    }
    void unsubscribe(std::vector<std::string>& data) {
        std::string username = data[0];

        _dataBase->getUser(username)->unsubscribe();
    }
    void updatePrice(std::vector<std::string>& data) {
        std::string titleBook = data[0];
        float price = std::stof(data[1]);

        _dataBase->updatePrice(titleBook, price);
    }
    void readBook(std::vector<std::string>& data) {
        std::string username = data[0];
        std::string titleBook = data[1];
        std::string author = _dataBase->getBook(titleBook).author;
        SuccessfulMessages::userReading(username, titleBook, author);
    }
    void listenBook(std::vector<std::string>& data) {
        std::string username = data[0];
        std::string titleBook = data[1];
        std::string author = _dataBase->getBook(titleBook).author;

        if (_dataBase->getPremiumUser(username) != nullptr)
            SuccessfulMessages::userListening(username, titleBook, author);
        else
            ErrorMessages::haveNotAccess();
    }
    void end() { exit(0); }

public:
    static OnlineBookStore* getInstance() {
        if(!store_instance)
            store_instance = new OnlineBookStore();
        return store_instance;
    }

    void submitFlowRequests() {
        std::stringstream ss;
        std::string input, word;

        while (true) {
            std::vector<std::string> data;
            std::getline(std::cin, input);
            ss.str(input);
            ss >> word;

            while (getline(ss, word, ' '))
                data.push_back(word);

            if (word == "createBook")
                createBook(data);
            else if (word == "createUser")
                createUser(data);
            else if (word == "subscribe")
                subscribe(data);
            else if (word == "unsubscribe")
                unsubscribe(data);
            else if (word == "updatePrice")
                updatePrice(data);
            else if (word == "readBook")
                readBook(data);
            else if (word == "listenBook")
                listenBook(data);
            else if (word == "end")
                end();

        }
    }
};


int main() {

    return 0;
}
