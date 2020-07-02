use benchmark;

grant select, insert, update, delete on benchmark.* to 'root'@'letmein' identified by 'root';

SET FOREIGN_KEY_CHECKS=0;

-- -------------------------
-- Table struct for rankings
-- -------------------------
DROP TABLE IF EXISTS `uservisits`;
CREATE TABLE `uservisits` (
  `source_ip` varchar(255) DEFAULT NULL,
  `dest_url` varchar(255) DEFAULT NULL,
  `visit_date` date DEFAULT NULL,
  `ad_revenue` float DEFAULT NULL,
  `user_agent` varchar(255) DEFAULT NULL,
  `country_code` varchar(255) DEFAULT NULL,
  `language_code` varchar(255) DEFAULT NULL,
  `search_word` varchar(255) DEFAULT NULL,
  `duration` int DEFAULT NULL
) ENGINE=InnoDB default CHARSET=utf8 ROW_FORMAT=COMPACT;
