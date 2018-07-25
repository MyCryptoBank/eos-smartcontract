#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

namespace mycryptobank {
	using std::string;
	using eosio::asset;
	using eosio::contract;
	using eosio::symbol_name;
	using eosio::multi_index;

	class mintabletoken : public contract {
		public:
			mintabletoken(account_name self) : contract(self){}
      
            /// Creates token with a symbol name for the specified issuer account.
            /// Throws if token with specified symbol already exists.
            /// @param issuer Account name of the token issuer.
            /// @param symbol Symbol code of the token.
            /// @param symbol precision Precision of the token.
            /// @param transferable Token trasferable status(true of false). If true is ts not neccessary to unfreeze after finishing minting.
			void create(account_name issuer, string symbol, uint8_t precision, bool transferable);

            /// Issues specified number of tokens with previously created symbol to the account name "to". 
            /// Requires authorization from the issuer.
            /// @param to Account name of tokens receiver
            /// @param value Number of tokens to issue for specified symbol (positive integer number)
            /// @param memo Action memo (max. 256 bytes)
			void issue(account_name to, asset value, string memo);

            /// Transfers specified number of tokens from account "from" to account "to".
            /// Throws if token with specified symbol code does not exist, or "from" balance is less than value.amount.
            /// @param from Account name of token owner
            /// @param to Account name of token receiver
            /// @param asset Amount of the tokens to transfer
            /// @param memo Action memo (max. 256 bytes)
			void transfer(account_name from, account_name to, asset value, string memo);

            /// Stops emission of the token with the specified symbol code.
            /// Requires authorization from the issuer.
            /// @param symbol Symbol code of token to stop minting.
			void finishissue(symbol_name symbol);

            /// Allows token transfers.
            /// Requires authorization from the issuer.
            /// @param symbol Symbol code of token to stop minting.
			void unfreeze(symbol_name symbol);

            /// Returns current total supply of specified token.
            /// @param symbol Symbol code of token to get total supply.
			inline asset get_supply(symbol_name symbol )const;

            /// Returns current ${holder} balance of specified token.
            /// @param holder Name of account to get balance.
            /// @param symbol Symbol code of token to get balance.
			inline asset get_balance( account_name holder, symbol_name symbol )const;


		private:
            /// Structure keeps information about the balance of tokens 
            /// for each symbol that is owned by an account. 
            /// This structure is stored in the multi_index table "holders".
            // @abi table holders i64
			struct holder {
        		asset balance;
				
				uint64_t primary_key() const { return balance.symbol.name(); }
      		};
            
            /// Account balance table
            /// Primary index:
            /// holder account name
      		typedef multi_index<N(holders), holder> holders;

            /// Structure keeps  system information about each issued token.
            /// Token keeps track of its supply, issuer, transfers ability and minting status.
            /// This structure is stored in the multi_index table "tokens_info".
            // @abi table token i64      		
            struct token_info {
        		asset supply;
        		account_name issuer;
        		bool frozen;
        		bool minting_finished;
        
        		uint64_t primary_key()const { return supply.symbol.name(); }
     		};

            /// Tokens info table
            /// Primary index:
            /// sumbol token symbol code.
     		typedef multi_index<N(tokens_info), token_info> tokens_info;

                        
     		void sub_balance(account_name from, asset value);

     		void add_balance(account_name holder, asset value, account_name ram_payer);

     		void add_supply(asset value);
	};

  	asset mintabletoken::get_supply(symbol_name symbol)const
    {
     	tokens_info tokens_info_table( _self, symbol);
     	auto existing = tokens_info_table.find(symbol);
     	eosio_assert(existing != tokens_info_table.end(), "unknown symbol");
     	const auto& token_info = *existing;
     	return token_info.supply;
    }

  	asset mintabletoken::get_balance( account_name holder, symbol_name symbol)const
    {
    	eosio_assert(is_account(holder), "holder account does not exist");
   		holders accounts_table( _self, holder);
   		const auto& account = accounts_table.get(symbol);
   		return account.balance;
    }

}
