#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>


static constexpr uint64_t token_symbol = S(4, MCB);

using eosio::asset;

class token: public eosio::contract {
   public:
      token( account_name self )
      :contract(self),_accounts( _self, _self),_system_info(_self, _self) {}

      // @abi action
      void transfer( account_name from, account_name to, asset quantity ) {
        require_auth( from );
        eosio_assert( quantity.symbol == token_symbol, "wrong symbol" );
        auto sym = quantity.symbol;
        auto sym_name = sym.name();
        auto _info = _system_info.find(sym_name);
        eosio_assert(!_info.frozen, "tokens are frozen");
        const auto& from_account = _accounts.get( from );
        eosio_assert( from_account.balance >= quantity, "insufficient funds" );
        _accounts.modify( from_account, from, [&]( auto& a ){ a.balance -= quantity; } );
        _increase_balance( from, to, quantity );
      }

      // @abi action 
      void init() {
          require_auth(_self);
          auto _info = _system_info.find(token_symbol);
          eosio_assert( _info == _system_info.end(), "already initilized");
          _system_info.emplace(_self, [&]( auto& _i ) {
            _i.symbol = token_symbol;
            _i.amount = 0;
            _i.frozen = true;
            _i.minting_finished = false;
          })
      }

      // @abi action 
      void finish_minting() {
          require_auth(_self);
          auto _info = _system_info.find(token_symbol);
          _system_info.modify(_info, _self, [&]( auto& _i ) {
            _i.frozen = false;
            _i.minting_finished = true;
          })
      }

      // @abi action
      void mint( account_name to, asset quantity ) {
        require_auth(_self );
        auto sym = quantity.symbol;
        auto sym_name = sym.name();
        auto _info = _system_info.find(sym_name);
        eosio_assert(quantity.symbol == token_symbol, "wrong symbol");
        eosio_assert(!_info.minting_finished, "minting finished");
        _increase_balance( _self, to, quantity );
        _system_info.modify(_self, [&]( auto& _i ) {
            _i.supply += quantity;
         });
      }


      asset get_balance(account_name owner) const {
        auto _account = _accounts.find( owner );
        return _account.balance;
      }

   private:
      // @abi table
      struct account {
         account_name    owner;
         eosio::asset  balance;

         uint64_t primary_key()const { return owner; }
      };

      // @abi table
      struct system_info {
         eosio::asset   supply;
         bool           frozen;
         bool minting_finished;

         uint64_t primary_key()const { return supply.symbol.name(); }
      };

      eosio::multi_index<N(account), account> _accounts;
      eosio::multi_index<N(system_info), system_info> _system_info;

      void _increase_balance(account_name payer, account_name to, asset quantity) {
        auto to_account = _accounts.find( to );
         if( to_account == _accounts.end() ) {
           _accounts.emplace( payer, [&]( auto& a ) {
              a.owner = to;
              a.balance = q;
           });
         } else {
          _accounts.modify( to_account, payer, [&]( auto& a ) {
              a.balance += quantity;
              eosio_assert( a.balance >= quantity, "balance overflow" );
           });
         }   
       }
};

EOSIO_ABI(token, (transfer)(init)(finish_minting)(mint))