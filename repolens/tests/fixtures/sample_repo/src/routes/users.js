const express = require("express");
const requireAuth = require("../middleware/authMiddleware");
const userService = require("../services/userService");

const router = express.Router();

router.get("/:id", requireAuth, async (req, res) => {
  try {
    const profile = await userService.getProfile(req.params.id, req.user);
    res.status(200).json(profile);
  } catch (err) {
    res.status(403).json({ error: err.message });
  }
});

module.exports = router;
