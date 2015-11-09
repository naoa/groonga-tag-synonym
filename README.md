# Groonga tag synonym plugin

Groongaの参照型カラムへのデータセット時に、投入データと参照先テーブルにある参照型の``synonym``カラムにあるキーと入れ替えることができます。

タグの同義語変換などに利用することができます。

### ```tag_synonym_add```

投入タグに対して同義語変換したいカラムにHOOKを登録します。
返り値はHOOKの数です。すでにIndexが貼られている場合は2になります。

```
tag_synonym_add --table Memos --column tags
[[0,0.0,0.0],[1]]
```

あらかじめタグテーブル(語彙表)にsynonymカラムを作り、変換後の値を入れておくことにより、データ投入時にその値に変換されます。以下の例では、Ruby=Rroongaと登録しておくことにより、RubyがRroongaに変換して登録されています。

```
plugin_register commands/tag_synonym
[[0,0.0,0.0],true]
table_create Tags TABLE_PAT_KEY ShortText
[[0,0.0,0.0],true]
column_create Tags synonym COLUMN_SCALAR Tags
[[0,0.0,0.0],true]
load --table Tags
[
{"_key": "Ruby", "synonym": "Rroonga"}
]
[[0,0.0,0.0],1]
table_create Memos TABLE_HASH_KEY ShortText
[[0,0.0,0.0],true]
column_create Memos tags COLUMN_VECTOR Tags
[[0,0.0,0.0],true]
tag_synonym_add --table Memos --column tags
[[0,0.0,0.0],[0]]
load --table Memos
[
{"_key": "Groonga", "tags": ["Groonga"]},
{"_key": "Rroonga", "tags": ["Groonga", "Ruby"]}
]
[[0,0.0,0.0],2]
select Memos --output_columns 'tags'
[[0,0.0,0.0],[[[2],[["tags","Tags"]],[["Groonga"]],[["Groonga","Rroonga"]]]]]
```

#### 制限
HOOKさせるカラムは他のテーブルへの参照型でないといけません。
また、参照先のテーブルに``synonym``カラムが必要です。``synonym``カラムは参照型である必要があります。

以下のように参照先のテーブルにTokenDelimitトークナイザを指定しておくことにより、入力時は空白区切の文字列であっても構いません。(Mroongaでの利用を想定)

```
table_create Tags TABLE_PAT_KEY ShortText --default_tokenizer TokenDelimit
load --table Memos
[
{"_key": "Groonga", "tags": "Groonga"},
{"_key": "Rroonga", "tags": "Groonga Ruby"}
]
[[0,0.0,0.0],2]
select Memos --output_columns 'tags'
[[0,0.0,0.0],[[[2],[["tags","Tags"]],[["Groonga"]],[["Groonga","Rroonga"]]]]]
```

### ```tag_synonym_delete```
カラムに登録したtag_synonymフックを解除します。

```
tag_synonym_delete --table Memos --column tags
[[0,0.0,0.0],[0]]
```


## Install

Install libgroonga-dev / groonga-devel

Build this command.

    % sh autogen.sh
    % ./configure
    % make
    % sudo make install

## Usage

Register `commands/tag_synonym`:

    % groonga DB
    > plugin_register commands/tag_synonym

Now, you can use `tag_synonym_add` and `tag_synonym_delete` command

## Author

Naoya Murakami naoya@createfield.com

## License

LGPL 2.1. See COPYING-LGPL-2.1 for details.
