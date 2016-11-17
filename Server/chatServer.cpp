//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "chat_message.hpp"
#include "HeartsGame.hpp"

int totalId = 0;
int roundId = 0;
int turns = 0;
int checker = 0;
std::vector<Player> lobby;
std::vector<HeartsGame> games;
std::vector<std::vector<Player>> gamePlayers;
int currentPlayer = 0;

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  int id;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void initGame(std::vector<Player> players)
  {
	HeartsGame newGame(players);
	games.push_back(newGame);
	games[games.size() - 1].play_Hearts();
	for (int i = 0; i < players.size(); i++)
	{
		for (auto participant : participants_)
		{
			if (participant->id == players[i].getId())
			{
				sendUpdate(participant, players[i].getIp(), games.size() - 1);
			}
		}
	}
  }

  void join(chat_participant_ptr participant, std::string ip)
  {
	participant->id = totalId++;
    participants_.insert(participant);
	Player p1(participant->id, ip);
	lobby.push_back(p1);
  }

  void sendRequest(chat_participant_ptr participant, char* message)
  {
	  std::string Msg = "REQUEST " + std::string(message);
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendLoginReply(chat_participant_ptr participant, bool login)
  {
	  std::string Msg = "Login Reply ";
	  if (login) Msg += "true";
	  else Msg += "false";
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendCreateAccountReply(chat_participant_ptr participant, bool login)
  {
	  std::string Msg = "CreateAccount Reply ";
	  if (login) Msg += "true";
	  else Msg += "false";
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendJoinPrivateReply(chat_participant_ptr participant, bool login)
  {
	  std::string Msg = "Join Private ";
	  if (login) Msg += "true";
	  else Msg += "false";
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendJoinPublicReply(chat_participant_ptr participant, bool login)
  {
	  std::string Msg = "Join Public ";
	  if (login) Msg += "true";
	  else Msg += "false";
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendCreatePrivateReply(chat_participant_ptr participant, bool login)
  {
	  std::string Msg = "Create Private ";
	  if (login) Msg += "true";
	  else Msg += "false";
	  sendMessageToPlayer(participant, Msg.data());
  }

  void sendMessageToPlayer(chat_participant_ptr participant, const char* data)
  {
	  chat_message msg;
	  msg.body_length(strlen(data));
	  std::memcpy(msg.body(), data, msg.body_length());
	  msg.encode_header();
	  participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant)
  {
	  for (auto participant_i : participants_)
		  if (participant_i->id > participant->id) participant_i->id--;
	  totalId--;
	  std::cout << "Player " << participant->id + 1 << " just left." << std::endl;
    participants_.erase(participant);
  }

  void handleMessage(std::string message, std::string ip)
  {
	  std::vector<std::string> messages;
	  std::string msg = "";
	  for (int i = 0; i < message.size(); i++)
	  {
		  if (message[i] == ' ')
		  {
			  messages.push_back(msg);
			  msg.clear();
		  }
		  else msg += message[i];
	  }
	  messages.push_back(msg);
	  if (messages[0] == "LOGIN") handleLogin(messages[1], messages[2],ip);
	  if (messages[0] == "CREATEACCOUNT") handleCreateAccount(messages[1], messages[2], ip);
	  if (messages[0] == "PASS")
	  {
		  Card card1((Suit)std::stoi(messages[1]), (Value)std::stoi(messages[2]));
		  Card card2((Suit)std::stoi(messages[3]), (Value)std::stoi(messages[4]));
		  Card card3((Suit)std::stoi(messages[5]), (Value)std::stoi(messages[6]));
		  handlePass(card1, card2, card3, ip);
	  }
	  if (messages[0] == "PLAY")
	  {
		  std::vector<Card> cards;
		  for (int i = 0; i < std::stoi(messages[1]); i++)
		  {
			  Card tmpCard((Suit)std::stoi(messages[i + 2]), (Value)std::stoi(messages[i + 3]));
		  }
		  handlePlay(cards, ip);
	  }
	  if (messages[0] == "BID") handleBid(messages[1],ip);
	  if (messages[0] == "QUIT") handleQuit(ip);
	 // if (messages[0] == "PLAYAGAIN") handlePlayAgain(ip);
	  if (messages[0] == "JOIN")
	  {
		  if (messages[1] == "PRIVATE") handleJoinPrivate(messages[2], ip);
		  if (messages[1] == "PUBLIC") handleJoinPublic(ip);
	  }
	  if (messages[0] == "CREATE")
	  {
		  if (messages[1] == "PRIVATE")
		  {
			  handleNewPrivate(messages[2], ip);
		  }
	  }
  }

  void handleNewPrivate(std::string passcode, std::string ip)
  {

  }

  void handleJoinPublic(std::string ip)
  {
	  bool gameFound = false;
	  int tmpPlayerId;
	  Player player(0,"0");
	  for (int i = 0; i < lobby.size(); i++)
	  {
		  if (lobby[i].getIp() == ip)
		  {
			  player = lobby[i];
			  tmpPlayerId = lobby[i].getId();
		  }
	  }
	  int gameCompleted = -1;
	  for (int i = 0; i < gamePlayers.size(); i++)
	  {
		  if (gamePlayers[i].size() < 4)
		  {
			  gamePlayers[i].push_back(player);
			  if (gamePlayers[i].size() == 4) gameCompleted = i;
			  gameFound = true;
		  }
	  }
	  if (!gameFound)
	  {
		  std::vector<Player> tmpPlayers;
		  tmpPlayers.push_back(player);
		  gamePlayers.push_back(tmpPlayers);
	  }
	  for (auto participant : participants_)
	  {
		  if (participant->id == tmpPlayerId)
			  sendJoinPublicReply(participant, true);
	  }
	  if (gameCompleted != -1)
	  {
		  initGame(gamePlayers[gameCompleted]);
	  }

  }

  void handleJoinPrivate(std::string passcode, std::string ip)
  {

  }

  void handleQuit(std::string ip)
  {
	  int gameIdx = getGameIdx(ip);
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  if (games[gameIdx].getPlayers()[i].getIp() == ip)
			  games[gameIdx].getPlayers()[i].setIp("AIE" + std::to_string(i));
	  }
  }

  void handleBid(std::string bid, std::string ip)
  {
	  int gameIdx = getGameIdx(ip);
	  int intBid = std::stoi(bid);
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  if (games[gameIdx].getPlayers()[i].getIp() == ip)
			  games[gameIdx].getPlayers()[i].getBid();
	  }
  }

  void handlePlay(std::vector<Card> cards, std::string ip)
  {
	  int gameIdx = getGameIdx(ip);
	  int nextTurn = 0;
	  int id;
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  if (games[gameIdx].getPlayers()[i].getIp() == ip)
		  {
			  nextTurn = games[gameIdx].playCard(cards, games[gameIdx].getPlayers()[i].getName());
			  
			  if(nextTurn == -1)
				  id = games[gameIdx].getPlayers()[i].getId();
			  else 
				  id = games[gameIdx].getPlayers()[(i+1)%games[gameIdx].getPlayers().size()].getId();
		  }
	  }

	  for (auto participant : participants_)
	  {
		  if (participant->id == id)
			  sendRequest(participant, "PLAY");
	  }

  }

  void handlePass(Card card1, Card card2, Card card3, std::string ip)
  {
	  int gameIdx = getGameIdx(ip);
	  std::vector<Card> cards;
	  cards.push_back(card1);
	  cards.push_back(card2);
	  cards.push_back(card3);
	  std::string name;
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  if (games[gameIdx].getPlayers()[i].getIp() == ip)
			  name = games[gameIdx].getPlayers()[i].getName();
	  }
	  games[gameIdx].setPassCards(cards, name);
  }

  void handleLogin(std::string name, std::string password, std::string ip)
  {
	  for (int i = 0; i < lobby.size(); i++)
	  {
		  if (lobby[i].getIp() == ip)
		  {
			  lobby[i].setName(name);
			  for (auto participant : participants_)
			  {
				  if (participant->id == lobby[i].getId())
					  sendLoginReply(participant, true);
			  }
		  }
	  }
  }

  void handleCreateAccount(std::string name, std::string password, std::string ip)
  {
	  for (int i = 0; i < lobby.size(); i++)
	  {
		  if (lobby[i].getIp() == ip)
		  {
			  lobby[i].setName(name);
			  for (auto participant : participants_)
			  {
				  if (participant->id == lobby[i].getId())
					  sendCreateAccountReply(participant, true);
			  }
		  }
	  }
  }
  
  int getGameIdx(std::string ip)
  {
	  int gameIdx = -1;
	  for (int i = 0; i < gamePlayers.size(); i++)
	  {
		  for (int j = 0; j < gamePlayers[i].size(); i++)
		  {
			  if (gamePlayers[i][j].getIp() == ip)
			  {
				  gameIdx = i;
				  return gameIdx;
			  }
		  }
	  }
	  return gameIdx;
  }

  void deliver(const chat_message& msg, std::string ip)
  {
	  int gameIdx = getGameIdx(ip);
	  
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

	//for (auto participant : participants_)
	//{
	std::string message = "";
	for (int i = 0; i < msg.body_length(); i++)
		message += msg.body()[i];// participant->deliver(msg);
	std::cout << message;
	handleMessage(message, ip);
	/*if (message[0] == '1')
	{
		for (auto participant : participants_)
		{
			char* message = "Here are your cards.";
			chat_message tmpMsg;
			tmpMsg.body_length(std::strlen(message));
			std::memcpy(tmpMsg.body(), message, tmpMsg.body_length());
			tmpMsg.encode_header();
			participant->deliver(tmpMsg);
		}
	}
	if (message.size() > 4)
	{
		
		std::string request = "";
		for (int i = 0; i < 4; i++)
		{
			request += message[i];
		}
		std::string values = "";
		for (int i = 5; i < message.size(); i++)
		{
			values += message[i];
		}	
		if (request == "PASS")
		{
			if (parsePass(values, ip) == 4)
			{
				game->passCards(roundId++);
				checker = 0;
				for (auto participant1 : participants_)
				{
					sendCards(participant1);
					sendUpdate(participant1);
					if (participant1->id == (game->findTwoOfClubs()))
					{
						sendRequest(participant1, "PLAY");
						currentPlayer = participant1->id;
					}
				}							
			}
		}
		if (request == "PLAY")
		{
			std::string tmpName;
			for (int i = 0; i < players.size(); i++)
			{
				if (players[i].getName() == ip + std::to_string(i))
					tmpName = players[i].getName();
			}
			int tmpId = game->playCard(values, tmpName);
			if (tmpId == -1)
			{
				for (auto participant1 : participants_)
				{
					if (participant1->id == currentPlayer)
					{
						sendRequest(participant1, "PLAY");
					}
				}
			}
			else
			{
				currentPlayer++;
				turns++;
				if (currentPlayer > 3) currentPlayer = 0;
				for (auto participant1 : participants_)
				{
					if (participant1->id == tmpId)
						sendCards(participant1);
					sendUpdate(participant1);
					if (participant1->id == currentPlayer && turns < 4)
						sendRequest(participant1, "PLAY");
					else if(turns >= 4)
					{
						turns = 0;
						checker = 0;
						currentPlayer = game->endTurn(currentPlayer);
						sendUpdate(participant1);
						if (game->getPlayers[0].getHand().size() == 0)
						{
							game->endRound();
							roundId++;
							game->play_Hearts();
							for (auto participant1 : participants_)
							{
								sendUpdate(participant1);
								sendCards(participant1);
								sendRequest(participant1, "PASS");
							}
						}
					}
				}
			}
		}
	}*/
	//}
	std::cout << std::endl;
  }

  void sendUpdate(chat_participant_ptr participant, std::string ip, int gameIdx)
  {
	  std::string message = "UPDATE CARDS ";
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  if (games[gameIdx].getPlayers()[i].getIp() == ip)
		  {
			  message += std::to_string(games[gameIdx].getPlayers()[i].getHand().size()) + ' ';
			  for (int j = 0; j < games[gameIdx].getPlayers()[i].getHand().size(); j++)
			  {
				  message += std::to_string(games[gameIdx].getPlayers()[i].getHand()[j].getSuit()) + ' ';
				  message += std::to_string(games[gameIdx].getPlayers()[i].getHand()[j].getValue()) + ' ';
			  }
			  break;
		  }
	  }
	  message += "CENTER ";
	  message += std::to_string(games[gameIdx].getCenterPile().size());
	  for (int i = 0; i < games[gameIdx].getCenterPile().size(); i++)
	  {
		  message += ' ' + std::to_string(games[gameIdx].getCenterPile()[i].getSuit());
		  message += ' ' + std::to_string(games[gameIdx].getCenterPile()[i].getValue());
	  }
	  message += " SCORES";
	  for (int i = 0; i < games[gameIdx].getPlayers().size(); i++)
	  {
		  message += ' ' + std::to_string(games[gameIdx].getPlayers()[i].getTotalScore());
	  }
	  sendMessageToPlayer(participant, message.data());
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
  int passes = 0;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start(std::string ip)
  {
    room_.join(shared_from_this(),ip);
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            room_.deliver(read_msg_, socket_.remote_endpoint().address().to_string());
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket_), room_)->start(socket_.remote_endpoint().address().to_string());
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    /*if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }*/

    boost::asio::io_service io_service;

    std::list<chat_server> servers;
    //for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), 8080);
      servers.emplace_back(io_service, endpoint);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}