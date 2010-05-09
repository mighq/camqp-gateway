BEGIN TRANSACTION;

INSERT INTO message (
	message_pk,
	sender,
	recipient,
	text
) VALUES (
	coalesce((SELECT max(message_pk)+1 FROM message), 1),
	"+420608077273",
	"+421908900601",
	"odpoved"
);

INSERT INTO message_outgoing (
	message_pk,
	date_created,
	date_sent,
	status
) VALUES (
	(SELECT message_pk FROM message ORDER BY message_pk DESC LIMIT 1),
	strftime('%s', 'now'),	-- created now
	NULL,					-- not sent yet
	0						-- to be sent
);

COMMIT;
