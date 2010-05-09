BEGIN TRANSACTION;

Create table "message_trash"
(
	"message_trash_pk" Integer NOT NULL,
	"message" BLOB NOT NULL,
	"date" Real NOT NULL,
	primary key ("message_trash_pk")
);

COMMIT;
