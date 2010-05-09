BEGIN TRANSACTION;

Create table "message"
(
	"message_pk" Integer NOT NULL,
	"sender" Text NOT NULL,
	"recipient" Text NOT NULL,
	"text" Text NOT NULL,
	primary key ("message_pk")
);

Create table "message_outgoing"
(
	"message_pk" Integer NOT NULL,
	"date_created" Real NOT NULL,
	"date_sent" Real,
	"status" Integer NOT NULL Default 0,
	primary key ("message_pk")
);

Create table "message_incoming"
(
	"message_pk" Integer NOT NULL,
	"date_received" Real NOT NULL,
	primary key ("message_pk")
);

COMMIT;
