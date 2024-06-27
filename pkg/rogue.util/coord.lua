local math3d = require "math3d"
local mathpkg   = import_package "ant.math"
local mu, mc = mathpkg.util, mathpkg.constant

local M = {}

local XZ_PLANE <const> = math3d.constant("v4", {0, 1, 0, 0})

function M.screen_to_world(viewport_mat, view_rect, x, y)
    local ndcpt = mu.pt2D_to_NDC({x, y}, view_rect)
    ndcpt[3] = 0
    local p0 = mu.ndc_to_world(viewport_mat, ndcpt)
    ndcpt[3] = 1
    local p1 = mu.ndc_to_world(viewport_mat, ndcpt)
    local _ , p = math3d.plane_ray(p0, math3d.sub(p0, p1), XZ_PLANE, true)
	return p
end

return M