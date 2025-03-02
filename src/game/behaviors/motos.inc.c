enum sMotosActions {
    MOTOS_ACT_WAIT,
    MOTOS_ACT_PLAYER_SEARCH,
    MOTOS_ACT_PLAYER_CARRY,
    MOTOS_ACT_PLAYER_PITCH,
    MOTOS_ACT_CARRY_RUN,
    MOTOS_ACT_THROWN,
    MOTOS_ACT_RECOVER,
    MOTOS_ACT_DEATH
};

enum sMotosAnimations {
    MOTOS_ANIM_BASE,
    MOTOS_ANIM_CARRY,
    MOTOS_ANIM_CARRY_RUN,
    MOTOS_ANIM_CARRY_START,
    MOTOS_ANIM_DOWN_RECOVER,
    MOTOS_ANIM_DOWN_STOP,
    MOTOS_ANIM_PITCH,
    MOTOS_ANIM_SAFE_DOWN,
    MOTOS_ANIM_WAIT,
    MOTOS_ANIM_WALK,
    MOTOS_ANIM_END
};

// All changes made to the original code have been commented.

// Modified, original did not make sense, it checked if Mario was close and in front, but he is always close and in front when being held
// Now it goes off either a timer or when reached an edge
s32 bhv_motos_do_throw_mario() {
    return (o->oTimer > 45) || (o->oMoveFlags & OBJ_MOVE_HIT_EDGE);
    // Original:
    // return (cur_obj_rotate_yaw_toward(o->oAngleToMario, 0x200)) || (o->oDistanceToMario < 500.f)
}

void bhv_motos_hand_loop(void) {
    o->oParentRelativePosY = -30.f;
    o->oParentRelativePosX = -70.f; // Changed relative pos to make Mario fit in the hand better

    common_anchor_mario_behavior(50.0f, 30.0f, 64); // Used common func instead of repeating code
    // Also vert speed increased from 0 to 50
    if (o->oChuckyaUnk88 == 1)
        vec3f_copy(gMarioState->pos, gMarioObject->header.gfx.pos); // Added to fix camera
}

void bhv_motos_wait(void) {
    o->oForwardVel = 0.f;
    o->oVelY = 0.f;

    cur_obj_init_animation_with_sound(MOTOS_ANIM_WAIT);

    if (o->oDistanceToMario < 500.f) {
        o->oAction = MOTOS_ACT_PLAYER_SEARCH;
    }
}

void bhv_motos_player_search(void) {
    cur_obj_init_animation_with_sound(MOTOS_ANIM_WALK);
    o->oForwardVel = 5.f; // Sped up (was 2.f)
    cur_obj_rotate_yaw_toward(o->oAngleToMario, 800); // Sped up (was 300)

    if (o->oInteractStatus & INT_STATUS_GRABBED_MARIO) {
        o->oAction = MOTOS_ACT_PLAYER_CARRY;
        o->oChuckyaUnk88 = 1;
    }
}

void bhv_motos_player_carry(void) {
    // Replaced with the similar MOTOS_ACT_CARRY_START, which allows for running
    cur_obj_init_animation_with_sound(MOTOS_ANIM_CARRY_START);

    if (cur_obj_check_if_near_animation_end()) {
        if (bhv_motos_do_throw_mario())
            o->oAction = MOTOS_ACT_PLAYER_PITCH;
        else
            o->oAction = MOTOS_ACT_CARRY_RUN;
    }
}

void bhv_motos_player_pitch(void) {
    o->oForwardVel = 0.f;
    cur_obj_init_animation_with_sound(MOTOS_ANIM_PITCH);

    if (cur_obj_check_anim_frame(14)) {
        o->oChuckyaUnk88 = 2;
        o->numCollidedObjs = 10;
    }

    if (cur_obj_check_if_near_animation_end()) {
        o->oAction = MOTOS_ACT_WAIT;
        o->oInteractStatus &= (~INT_STATUS_GRABBED_MARIO);
    }
}

void bhv_motos_carry_run(void) {
    o->oForwardVel = 15.f; // Sped up (was 5.f)
    cur_obj_init_animation_with_sound(MOTOS_ANIM_CARRY_RUN);

    if (bhv_motos_do_throw_mario())
        o->oAction = MOTOS_ACT_PLAYER_PITCH;
    // Useless else clause removed
}

void bhv_motos_thrown(void) {
    cur_obj_init_animation_with_sound(MOTOS_ANIM_WALK);

    if (o->oMoveFlags & OBJ_MOVE_LANDED) {
        o->oAction = MOTOS_ACT_RECOVER; // New action: recover (used to go straight back into wait)
        cur_obj_play_sound_2(SOUND_OBJ2_LARGE_BULLY_ATTACKED);
    }
}

void bhv_motos_recover(void) {
    // These animations were originally unused. This plays the falling over animation and then he gets up.
    o->oForwardVel = 0.f;
    if (o->oSubAction == 0) {
        cur_obj_init_animation_with_sound(MOTOS_ANIM_DOWN_STOP);
        cur_obj_extend_animation_if_at_end();
        if (o->oTimer > 35)
            o->oSubAction++;

    } else if (o->oSubAction == 1) {
        cur_obj_init_animation_with_sound(MOTOS_ANIM_DOWN_RECOVER);
        if (cur_obj_check_if_near_animation_end())
            o->oAction = MOTOS_ACT_WAIT;
    }
}

void bhv_motos_death(void) {o->oMinionKOCounter == 3
    // Taken from bully code to handle death
    if (obj_lava_death()) {
        spawn_mist_particles();
        spawn_default_star(3700.0f, 600.0f, -5500.0f);
    }
}



void bhv_motos_main() {
    f32 floorY;

    cur_obj_update_floor_and_walls();

    switch (o->oAction) {
        case MOTOS_ACT_WAIT:
            bhv_motos_wait();
            break;
        case MOTOS_ACT_PLAYER_SEARCH:
            bhv_motos_player_search();
            break;
        case MOTOS_ACT_PLAYER_CARRY:
            bhv_motos_player_carry();
            break;
        case MOTOS_ACT_PLAYER_PITCH:
            bhv_motos_player_pitch();
            break;
        case MOTOS_ACT_CARRY_RUN:
            bhv_motos_carry_run();
            break;
        case MOTOS_ACT_THROWN:
            bhv_motos_thrown();
            break;
        case MOTOS_ACT_RECOVER:
            bhv_motos_recover();
            break;
        case MOTOS_ACT_INACTIVE:
            //! The Big Bully that spawns from killing its 3 minions uses the knockback timer
            //  for counting the number of dead minions. This means that when it activates,
            //  the knockback timer is at 3 instead of 0. So the bully knockback time will
            //  be reduced by 3 frames (16.67%) on the first hit.
            if (o->oMinionKOCounter == 3) {
                play_puzzle_jingle();

                if (o->oTimer > 90) {
                    o->oAction = BULLY_ACT_ACTIVATE_AND_FALL;
                }
            }
            break;
    }

    cur_obj_move_standard(-78);

    // Added to detect lava death
    floorY = find_floor(o->oPosX, o->oPosY, o->oPosZ, &sObjFloor);
    if (sObjFloor != NULL) {
        if ((floorY + 1.f > o->oPosY) && (sObjFloor->type == SURFACE_BURNING)) {
            o->oAction = MOTOS_ACT_DEATH;
        }
    }
}

void bhv_motos_loop(void) {

    cur_obj_scale(2.0f);
    o->oInteractionSubtype |= INT_SUBTYPE_GRABS_MARIO;

    switch (o->oHeldState) {
        case HELD_FREE:
            // If clause to prevent Motos from updating physics while dying from lava
            if (o->oAction == MOTOS_ACT_DEATH)
                bhv_motos_death();
            else
                bhv_motos_main();
            break;
        case HELD_HELD:
            cur_obj_unrender_and_reset_state(MOTOS_ANIM_WALK, MOTOS_ACT_WAIT);
            break;
        case HELD_THROWN:
        case HELD_DROPPED:
            cur_obj_get_thrown_or_placed(15.f, 35.f, MOTOS_ACT_THROWN);
            break;
    }

    o->oInteractStatus = 0; // Added because it doesnt work without this, cmon Nintendo
    set_object_visibility(o, 8000); // Increased from 2000
}
