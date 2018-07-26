#include "mycryptobank.hpp"


namespace mycryptobank {

    using eosio::string_to_symbol;

    // @abi action
    void mintabletoken::create(account_name issuer, string symbol, uint8_t precision, bool frozen)
    {
        require_auth(_self);
        eosio_assert(is_account(issuer), "issuer account does not exist");
        
        asset supply(0, string_to_symbol(precision, symbol.c_str()));
        auto _symbol = supply.symbol;

        eosio_assert(_symbol.is_valid(), "invalid symbol name" );
        eosio_assert(_symbol.is_valid(), "invalid supply");
        
        auto symbol_name = _symbol.name();
        tokens_info tokens_info_table(_self, symbol_name);
        auto existing = tokens_info_table.find(symbol_name);
        eosio_assert(existing != tokens_info_table.end(), "token with the same symbol already exists");
        
        tokens_info_table.emplace(_self, [&](auto& token_info) {
            token_info.supply = supply;
            token_info.issuer = issuer;
            token_info.frozen = frozen;
            token_info.minting_finished = false;
        });
    }

    // @abi action
    void mintabletoken::issue(account_name to, asset value, string memo)
    {
        eosio_assert(is_account(to), "to account does not exist");
        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    
        auto symbol = value.symbol;
        auto symbol_name = symbol.name();
        
        tokens_info tokens_info_table( _self, symbol_name);
        auto existing = tokens_info_table.find(symbol_name);
        eosio_assert(existing != tokens_info_table.end(), "unknown symbol, create token before issuance");
        const auto& token_sysinfo = *existing;
        eosio_assert(token_sysinfo.minting_finished == false, "minting is finished");

        require_auth(token_sysinfo.issuer);

        eosio_assert(value.is_valid(), "invalid issuance value");
        eosio_assert(value.amount > 0, "negative issuance amount");

        eosio_assert(value.symbol == token_sysinfo.supply.symbol, "wrong precision");

        add_supply(value);
        add_balance(to, value, token_sysinfo.issuer);

        if( to != token_sysinfo.issuer ) {
            SEND_INLINE_ACTION(*this, transfer, {token_sysinfo.issuer,N(active)}, {token_sysinfo.issuer, to, value, memo});
        }
    }

    // @abi action
    void mintabletoken::transfer(account_name from, account_name to, asset value, string memo)
    {
        eosio_assert(from != to, "self transfers are not allowed");
        require_auth(from);
        eosio_assert(is_account(to), "dectination account does not exist");
        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes" );

        auto symbol = value.symbol.name();
        tokens_info tokens_info_table( _self, symbol);
        const auto& token_info = tokens_info_table.get(symbol);

        require_recipient(from);
        require_recipient(to);

        eosio_assert(value.is_valid(), "invalid quantity" );
        eosio_assert(value.amount > 0, "negative transfer amount" );
        eosio_assert(value.symbol == token_info.supply.symbol, "wrong precision" );
        
        sub_balance(from, value);
        add_balance(to, value, from );
        
    }

    // @abi action
    void mintabletoken::finishissue(symbol_name symbol)
    {
        tokens_info tokens_info_table( _self, symbol);
        auto existing = tokens_info_table.find(symbol);
        eosio_assert(existing != tokens_info_table.end(), "unknown symbol");
        const auto& token_sysinfo = *existing;
        tokens_info_table.modify(token_sysinfo, token_sysinfo.issuer, [&]( auto& _token_sysinfo) {
            _token_sysinfo.minting_finished = true;
        });
    }

    // @abi action
    void mintabletoken::unfreeze(symbol_name symbol)
    {   
        tokens_info tokens_info_table( _self, symbol);
        auto existing = tokens_info_table.find(symbol);
        eosio_assert(existing != tokens_info_table.end(), "unknown symbol");
        const auto& token_sysinfo = *existing;
        require_auth(token_sysinfo.issuer);
        tokens_info_table.modify(token_sysinfo, token_sysinfo.issuer, [&]( auto& _token_sysinfo) {
            _token_sysinfo.frozen = false;
        });
    }

    void mintabletoken::sub_balance(account_name holder, asset value)
    {
        holders accounts_table( _self, holder);
        const auto& account_info = accounts_table.get(value.symbol.name(), "account not found");
        eosio_assert(account_info.balance.amount > value.amount, "insufficient funds");

        if(account_info.balance.amount == value.amount) {
            accounts_table.erase(account_info);
        } else {
            accounts_table.modify(account_info, holder, [&]( auto& _account_info) {
            _account_info.balance -= value;
            });
        }
    }

    void mintabletoken::add_balance( account_name holder, asset value, account_name ram_payer ) 
    {
        holders accounts_table( _self, holder);
        auto account_info = accounts_table.find(value.symbol.name());
        if(account_info == accounts_table.end() ) {
            accounts_table.emplace(ram_payer, [&]( auto& _account_info){
                _account_info.balance = value;
            });
        } else {
            accounts_table.modify(account_info, 0, [&]( auto& _account_info) {
                _account_info.balance += value;
            });
        }
    }

    void mintabletoken::add_supply(asset value) 
    {
        auto symbol_name = value.symbol.name();
        tokens_info tokens_info_table( _self, symbol_name);
        auto current_token_info = tokens_info_table.find(symbol_name);

        tokens_info_table.modify(current_token_info, 0, [&]( auto& token_info) {
            token_info.supply += value;
        });
    }
    
    EOSIO_ABI(mintabletoken, (create)(issue)(transfer)(unfreeze)(finishissue))
}
