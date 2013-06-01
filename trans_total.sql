CREATE VIEW trans_total as
SELECT 
  trans_main.pk_uid
, trans_main.id_account
, account.account_name AS relate_account
, trans_main.date_trans
, trans_main.comment
, trans_main.amount
, COALESCE(
               (SELECT SUM(trans_total.amount)+trans_main.amount 
                 FROM trans trans_total 
                 WHERE 
                     trans_total.id_account=trans_main.id_account 
                     AND (trans_total.date_trans<trans_main.date_trans 
                                  OR (trans_total.date_trans=trans_main.date_trans AND trans_total.pk_uid<trans_main.pk_uid)
                                 )
                 ) 
      , trans_main.amount) AS total 
FROM 
  trans trans_main LEFT JOIN trans trans_relate ON trans_main.id_relate=trans_relate.pk_uid
    LEFT JOIN account ON trans_relate.id_account=account.pk_uid
