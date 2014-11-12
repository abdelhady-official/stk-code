//  SuperTuxKart - a fun racing game with go-kart
//
//  Copyright (C) 2006-2013 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "config/player_manager.hpp"
#include "guiengine/widgets/kart_stats_widget.hpp"
#include "guiengine/widgets/model_view_widget.hpp"
#include "guiengine/widgets/player_kart_widget.hpp"
#include "guiengine/widgets/player_name_spinner.hpp"
#include "input/input_device.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "online/online_profile.hpp"
#include "states_screens/kart_selection.hpp"
#include <IGUIEnvironment.h>

static const char RANDOM_KART_ID[] = "randomkart";

using namespace GUIEngine;

PlayerKartWidget::PlayerKartWidget(KartSelectionScreen* parent,
                                   StateManager::ActivePlayer* associated_player,
                                   Online::OnlineProfile* associated_user,
                                   core::recti area, const int player_id,
                                   std::string kart_group,
                                   const int irrlicht_widget_id) : Widget(WTYPE_DIV)
{
#ifdef DEBUG
    if (associated_player)
        assert(associated_player->ok());
    m_magic_number = 0x33445566;
#endif
    m_ready_text = NULL;
    m_parent_screen = parent;

    m_associated_user = associated_user;
    m_associated_player = associated_player;
    x_speed = 1.0f;
    y_speed = 1.0f;
    w_speed = 1.0f;
    h_speed = 1.0f;
    m_ready = false;
    m_handicapped = false;
    m_not_updated_yet = true;

    m_irrlicht_widget_id = irrlicht_widget_id;

    m_player_id = player_id;
    m_properties[PROP_ID] = StringUtils::insertValues("@p%i", m_player_id);

    setSize(area.UpperLeftCorner.X, area.UpperLeftCorner.Y,
            area.getWidth(), area.getHeight()               );
    target_x = m_x;
    target_y = m_y;
    target_w = m_w;
    target_h = m_h;

    // ---- Player identity spinner
    m_player_ident_spinner = NULL;

    m_player_ident_spinner = new PlayerNameSpinner(parent, m_player_id);
    m_player_ident_spinner->m_x = player_name_x;
    m_player_ident_spinner->m_y = player_name_y;
    m_player_ident_spinner->m_w = player_name_w;
    m_player_ident_spinner->m_h = player_name_h;

    // ---- KartStatsWidget
    m_kart_stats = NULL;

    // area for the stats widget
    core::recti statsArea;
    if (!parent->m_multiplayer)
    {
        statsArea = core::recti(m_kart_stats_x,
                                m_kart_stats_y,
                                m_kart_stats_x + m_kart_stats_w,
                                m_kart_stats_y + m_kart_stats_h);
    }
    else
    {
        statsArea = core::recti(m_x , m_y + m_h/2,
                                m_x + m_w, m_y + m_h);
    }


    m_kart_stats = new GUIEngine::KartStatsWidget(statsArea, player_id, kart_group,
                       m_parent_screen->m_multiplayer,
                       !m_parent_screen->m_multiplayer || parent->m_kart_widgets.size() == 0);
    m_kart_stats->m_properties[PROP_ID] = StringUtils::insertValues("@p%i_stats", m_player_id);
    m_children.push_back(m_kart_stats);

    if (parent->m_multiplayer && associated_player)
    {
        if (associated_player->getDevice()->getType() == DT_KEYBOARD)
        {
            m_player_ident_spinner->setBadge(KEYBOARD_BADGE);
        }
        else if (associated_player->getDevice()->getType() == DT_GAMEPAD)
        {
            m_player_ident_spinner->setBadge(GAMEPAD_BADGE);
        }
    }
    else if (m_associated_user) // online user, FIXME is that useful ?
    {
        m_player_ident_spinner->setBadge(OK_BADGE);
    }

    if (irrlicht_widget_id == -1)
    {
        m_player_ident_spinner->m_tab_down_root = g_root_id;
    }

    spinnerID = StringUtils::insertValues("@p%i_spinner", m_player_id);

    m_player_ident_spinner->m_properties[PROP_ID] = spinnerID;
    if (parent->m_multiplayer)
    {
        const int player_amount = PlayerManager::get()->getNumPlayers();
        m_player_ident_spinner->m_properties[PROP_MIN_VALUE] = "0";
        m_player_ident_spinner->m_properties[PROP_MAX_VALUE] =
            StringUtils::toString(player_amount-1);
        m_player_ident_spinner->m_properties[PROP_WRAP_AROUND] = "true";
    }
    else
    {
        m_player_ident_spinner->m_properties[PROP_MIN_VALUE] = "0";
        m_player_ident_spinner->m_properties[PROP_MAX_VALUE] = "0";
    }

    //m_player_ident_spinner->m_event_handler = this;
    m_children.push_back(m_player_ident_spinner);

    // ----- Kart model view
    m_model_view = new ModelViewWidget();

    m_model_view->m_x = model_x;
    m_model_view->m_y = model_y;
    m_model_view->m_w = model_w;
    m_model_view->m_h = model_h;
    m_model_view->m_properties[PROP_ID] =
        StringUtils::insertValues("@p%i_model", m_player_id);
    //m_model_view->setParent(this);
    m_children.push_back(m_model_view);

    // Init kart model
    const std::string default_kart = UserConfigParams::m_default_kart;
    const KartProperties* props =
        kart_properties_manager->getKart(default_kart);

    if(!props)
    {
        // If the default kart can't be found (e.g. previously a addon
        // kart was used, but the addon package was removed), use the
        // first kart as a default. This way we don't have to hardcode
        // any kart names.
        int id = kart_properties_manager->getKartByGroup(kart_group, 0);
        if (id == -1)
        {
            props = kart_properties_manager->getKartById(0);
        }
        else
        {
            props = kart_properties_manager->getKartById(id);
        }

        if(!props)
            Log::fatal("KartSelectionScreen", "Can't find default "
                       "kart '%s' nor any other kart.",
                       default_kart.c_str());
    }
    m_kartInternalName = props->getIdent();

    const KartModel &kart_model = props->getMasterKartModel();
    
    float scale = 35.0f;
    if (kart_model.getLength() > 1.45f)
    {
        // if kart is too long, size it down a bit so that it fits
        scale = 30.0f;
    }

    m_model_view->addModel( kart_model.getModel(), Vec3(0,0,0),
                            Vec3(scale, scale, scale),
                            kart_model.getBaseFrame() );
    m_model_view->addModel( kart_model.getWheelModel(0),
                            kart_model.getWheelGraphicsPosition(0) );
    m_model_view->addModel( kart_model.getWheelModel(1),
                            kart_model.getWheelGraphicsPosition(1) );
    m_model_view->addModel( kart_model.getWheelModel(2),
                            kart_model.getWheelGraphicsPosition(2) );
    m_model_view->addModel( kart_model.getWheelModel(3),
                            kart_model.getWheelGraphicsPosition(3) );
    for(size_t i=0 ; i < kart_model.getSpeedWeightedObjectsCount() ; i++)
    {
        const SpeedWeightedObject& obj = kart_model.getSpeedWeightedObject((int)i);
        m_model_view->addModel(obj.m_model, obj.m_position);
    }
    m_model_view->setRotateContinuously( 35.0f );

    // ---- Kart name label
    m_kart_name = new LabelWidget(true, true);
    m_kart_name->setText(props->getName(), false);
    m_kart_name->m_properties[PROP_TEXT_ALIGN] = "center";
    m_kart_name->m_properties[PROP_ID] =
        StringUtils::insertValues("@p%i_kartname", m_player_id);
    m_kart_name->m_x = kart_name_x;
    m_kart_name->m_y = kart_name_y;
    m_kart_name->m_w = kart_name_w;
    m_kart_name->m_h = kart_name_h;
    m_children.push_back(m_kart_name);
}   // PlayerKartWidget
// ------------------------------------------------------------------------

PlayerKartWidget::~PlayerKartWidget()
{
    if (GUIEngine::getFocusForPlayer(m_player_id) == this)
    {
        GUIEngine::focusNothingForPlayer(m_player_id);
    }

    if (m_player_ident_spinner != NULL)
    {
        m_player_ident_spinner->setListener(NULL);

        if (m_player_ident_spinner->getIrrlichtElement() != NULL)
        {
            m_player_ident_spinner->getIrrlichtElement()->remove();
        }
    }

    if (m_model_view->getIrrlichtElement() != NULL)
        m_model_view->getIrrlichtElement()->remove();

    if (m_kart_name->getIrrlichtElement() != NULL)
        m_kart_name->getIrrlichtElement()->remove();
    getCurrentScreen()->manualRemoveWidget(this);

#ifdef DEBUG
    m_magic_number = 0xDEADBEEF;
#endif
}   // ~PlayerKartWidget

// ------------------------------------------------------------------------
/** Called when players are renumbered (changes the player ID) */
void PlayerKartWidget::setPlayerID(const int newPlayerID)
{
    assert(m_magic_number == 0x33445566);

    if (StateManager::get()->getActivePlayer(newPlayerID)
            != m_associated_player)
    {
        Log::error("KartSelectionScreen",  "Internal "
                   "inconsistency, PlayerKartWidget has IDs and "
                   "pointers that do not correspond to one player");
        Log::fatal("KartSelectionScreen", "    Player: %p  -  Index: %d  -  m_associated_player: %p",
                   StateManager::get()->getActivePlayer(newPlayerID),
                   newPlayerID, m_associated_player);
    }

    // Remove current focus, but remember it
    Widget* focus = GUIEngine::getFocusForPlayer(m_player_id);
    GUIEngine::focusNothingForPlayer(m_player_id);

    // Change the player ID
    m_player_id = newPlayerID;
    if (!m_ready)
        m_player_ident_spinner->setID(m_player_id);
    m_kart_stats->setDisplayText(m_player_id == 0);
    // restore previous focus, but with new player ID
    if (focus != NULL) focus->setFocusForPlayer(m_player_id);

    if (m_player_ident_spinner != NULL)
    {
        m_player_ident_spinner->setID(m_player_id);
    }
}   // setPlayerID

// ------------------------------------------------------------------------
/** Returns the ID of this player */
int PlayerKartWidget::getPlayerID() const
{
    assert(m_magic_number == 0x33445566);
    return m_player_id;
}   // getPlayerID

// ------------------------------------------------------------------------
/** Add the widgets to the current screen */
void PlayerKartWidget::add()
{
    assert(m_magic_number == 0x33445566);

    assert(KartSelectionScreen::getRunningInstance()
           ->m_kart_widgets.contains(this));
    if (m_associated_player) // if player is local
    {
        bool mineInList = false;
        for (unsigned int p=0; p<StateManager::get()->activePlayerCount(); p++)
        {
#ifdef DEBUG
            assert(StateManager::get()->getActivePlayer(p)->ok());
#endif
            if (StateManager::get()->getActivePlayer(p) == m_associated_player)
            {
                mineInList = true;
            }
        }
        assert(mineInList);
    }

    // the first player will have an ID of its own to allow for keyboard
    // navigation despite this widget being added last
    if (m_irrlicht_widget_id != -1)
        m_player_ident_spinner->m_reserved_id = m_irrlicht_widget_id;
    else
        m_player_ident_spinner->m_reserved_id = Widget::getNewNoFocusID();

    m_player_ident_spinner->add();
    m_player_ident_spinner->getIrrlichtElement()->setTabStop(false);
    m_player_ident_spinner->setListener(this);
    m_kart_stats->add();
    m_model_view->add();
    m_kart_name->add();

    m_model_view->update(0);

    m_player_ident_spinner->clearLabels();

    irr::core::stringw name; // name of the player
    if (m_associated_player)
        name = m_associated_player->getProfile()->getName();
    if (m_associated_user)
        name = m_associated_user->getUserName();

    if (m_parent_screen->m_multiplayer)
    {
        const int player_amount = PlayerManager::get()->getNumPlayers();
        for (int n=0; n<player_amount; n++)
        {
            core::stringw name = PlayerManager::get()->getPlayer(n)->getName();
            m_player_ident_spinner->addLabel(translations->fribidize(name));
            if (UserConfigParams::m_per_player_difficulty)
                // The second player is the same, but with handicap
                m_player_ident_spinner->addLabel(translations->fribidize(name));
        }

        // select the right player profile in the spinner
        m_player_ident_spinner->setValue(name);
    }
    else
    {
        m_player_ident_spinner->addLabel(name);
        m_player_ident_spinner->setVisible(false);
    }

    // Add anchor badge if the player is handicapped
    int spinner_value = m_player_ident_spinner->getValue();

    assert(m_player_ident_spinner->getStringValue() == name);
}   // add

// ------------------------------------------------------------------------
/** Get the associated ActivePlayer object*/
StateManager::ActivePlayer* PlayerKartWidget::getAssociatedPlayer()
{
    assert(m_magic_number == 0x33445566);
    return m_associated_player;
}   // getAssociatedPlayer

// ------------------------------------------------------------------------
/** Starts a 'move/resize' animation, by simply passing destination coords.
 *  The animation will then occur on each call to 'onUpdate'. */
void PlayerKartWidget::move(const int x, const int y, const int w, const int h)
{
    assert(m_magic_number == 0x33445566);
    target_x = x;
    target_y = y;
    target_w = w;
    target_h = h;

    x_speed = abs( m_x - x ) / 300.0f;
    y_speed = abs( m_y - y ) / 300.0f;
    w_speed = abs( m_w - w ) / 300.0f;
    h_speed = abs( m_h - h ) / 300.0f;
}   // move

// ------------------------------------------------------------------------
/** Call when player confirmed his identity and kart */
void PlayerKartWidget::markAsReady()
{
    assert(m_magic_number == 0x33445566);
    if (m_ready) return; // already ready

    m_ready = true;

    stringw playerNameString = m_player_ident_spinner->getStringValue();
    core::rect<s32> rect(core::position2di(m_player_ident_spinner->m_x,
                                           m_player_ident_spinner->m_y),
                         core::dimension2di(m_player_ident_spinner->m_w,
                                            m_player_ident_spinner->m_h));
    // 'playerNameString' is already fribidize, so we need to use
    // 'insertValues' and not _("...", a) so it's not flipped again
    m_ready_text =
        GUIEngine::getGUIEnv()->addStaticText(
            StringUtils::insertValues(_("%s is ready"),
                                      playerNameString).c_str(),
            rect                    );
    m_ready_text->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER );

    m_children.remove(m_player_ident_spinner);
    m_player_ident_spinner->setListener(NULL);
    m_player_ident_spinner->getIrrlichtElement()->remove();
    m_player_ident_spinner->elementRemoved();
    delete m_player_ident_spinner;
    m_player_ident_spinner = NULL;

    SFXManager::get()->quickSound( "wee" );

    m_model_view->setRotateTo(30.0f, 1.0f);

    player_name_w = 0;

    m_model_view->setBadge(OK_BADGE);
}   // markAsReady

// ------------------------------------------------------------------------
/** \return Whether this player confirmed his kart and indent selection */
bool PlayerKartWidget::isReady()
{
    assert(m_magic_number == 0x33445566);
    return m_ready;
}   // isReady

// ------------------------------------------------------------------------
/** \return Whether this player is handicapped or not */
bool PlayerKartWidget::isHandicapped()
{
    assert(m_magic_number == 0x33445566);
    return m_handicapped;
}   // isHandicapped

// -------------------------------------------------------------------------
/** Updates the animation (moving/shrinking/etc.) */
void PlayerKartWidget::onUpdate(float delta)
{
    assert(m_magic_number == 0x33445566);
    if (target_x == m_x && target_y == m_y &&
            target_w == m_w && target_h == m_h) return;

    int move_step = (int)(delta*1000.0f);

    // move x towards target
    if (m_x < target_x)
    {
        m_x += (int)(move_step*x_speed);
        // don't move to the other side of the target
        if (m_x > target_x) m_x = target_x;
    }
    else if (m_x > target_x)
    {
        m_x -= (int)(move_step*x_speed);
        // don't move to the other side of the target
        if (m_x < target_x) m_x = target_x;
    }

    // move y towards target
    if (m_y < target_y)
    {
        m_y += (int)(move_step*y_speed);
        // don't move to the other side of the target
        if (m_y > target_y) m_y = target_y;
    }
    else if (m_y > target_y)
    {
        m_y -= (int)(move_step*y_speed);
        // don't move to the other side of the target
        if (m_y < target_y) m_y = target_y;
    }

    // move w towards target
    if (m_w < target_w)
    {
        m_w += (int)(move_step*w_speed);
        // don't move to the other side of the target
        if (m_w > target_w) m_w = target_w;
    }
    else if (m_w > target_w)
    {
        m_w -= (int)(move_step*w_speed);
        // don't move to the other side of the target
        if (m_w < target_w) m_w = target_w;
    }
    // move h towards target
    if (m_h < target_h)
    {
        m_h += (int)(move_step*h_speed);
        // don't move to the other side of the target
        if (m_h > target_h) m_h = target_h;
    }
    else if (m_h > target_h)
    {
        m_h -= (int)(move_step*h_speed);
        // don't move to the other side of the target
        if (m_h < target_h) m_h = target_h;
    }

    setSize(m_x, m_y, m_w, m_h);

    if (m_player_ident_spinner != NULL)
    {
        m_player_ident_spinner->move(player_name_x,
                                     player_name_y,
                                     player_name_w,
                                     player_name_h );
    }
    if (m_ready_text != NULL)
    {
        m_ready_text->setRelativePosition(
            core::recti(core::position2di(player_name_x, player_name_y),
                        core::dimension2di(player_name_w, player_name_h))           );
    }
    if (!m_parent_screen->m_multiplayer)
    {
        m_kart_stats->move(m_kart_stats_x,
                           m_kart_stats_y,
                           m_kart_stats_w,
                           m_kart_stats_h);
    }
    else
    {
        m_kart_stats->move(m_x, m_y + m_h/2,
                           m_w, m_h/2);
    }


    m_model_view->move(model_x,
                       model_y,
                       model_w,
                       model_h);

    m_kart_name->move(kart_name_x,
                      kart_name_y,
                      kart_name_w,
                      kart_name_h);

    // When coming from the overworld, we must rebuild the preview scene at
    // least once, since the scene is being cleared by leaving the overworld
    if (m_not_updated_yet)
    {
        m_model_view->clearRttProvider();
        m_not_updated_yet = false;
    }
}   // onUpdate

// -------------------------------------------------------------------------
/** Event callback */
GUIEngine::EventPropagation PlayerKartWidget::transmitEvent(Widget* w,
                                              const std::string& originator,
                                              const int m_player_id          )
{
    assert(m_magic_number == 0x33445566);
    // if it's declared ready, there is really nothing to process
    if (m_ready) return EVENT_LET;

    //std::cout << "= kart selection :: transmitEvent "
    // << originator << " =\n";

    std::string name = w->m_properties[PROP_ID];

    //std::cout << "    (checking if that's me: I am "
    // << spinnerID << ")\n";

    // update player profile when spinner changed
    if (originator == spinnerID)
    {
        if(UserConfigParams::logGUI())
        {
            Log::info("[KartSelectionScreen]", "Identity changed "
                      "for player %s : %s",m_player_id,
                      irr::core::stringc(
                          m_player_ident_spinner->getStringValue()
                          .c_str()).c_str());
        }

        if (m_parent_screen->m_multiplayer)
        {
            int spinner_value = m_player_ident_spinner->getValue();
            PlayerProfile* profile = PlayerManager::get()->getPlayer(
                UserConfigParams::m_per_player_difficulty ? spinner_value / 2 : spinner_value);
            m_associated_player->setPlayerProfile(profile);
            if(UserConfigParams::m_per_player_difficulty && spinner_value % 2 != 0)
            {
                m_handicapped = true;
                m_model_view->setBadge(ANCHOR_BADGE);
            }
            else
            {
                m_handicapped = false;
                m_model_view->unsetBadge(ANCHOR_BADGE);
            }
        }
    }

    return EVENT_LET; // continue propagating the event
}   // transmitEvent

// -------------------------------------------------------------------------
/** Sets the size of the widget as a whole, and placed children widgets
 * inside itself */
void PlayerKartWidget::setSize(const int x, const int y, const int w, const int h)
{
    assert(m_magic_number == 0x33445566);
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;

    // -- sizes
    player_name_h = 40;
    player_name_w = std::min(400, w);

    kart_name_w = w;
    kart_name_h = 25;

    // for shrinking effect
    if (h < 175)
    {
        const float factor = h / 175.0f;
        kart_name_h   = (int)(kart_name_h*factor);
        player_name_h = (int)(player_name_h*factor);
    }

    // --- layout
    player_name_x = x + w/2 - player_name_w/2;
    player_name_y = y;

    if (m_parent_screen->m_multiplayer)
    {
        const int modelMaxHeight = (h - kart_name_h - player_name_h)/2;
        const int modelMaxWidth =  w;
        const int bestSize = std::min(modelMaxWidth, modelMaxHeight);
        model_x = x + w/2 - (int)(bestSize/2);
        model_y =  y + player_name_h;
        model_w = bestSize;
        model_h = bestSize;

        m_kart_stats_w = model_w;
        m_kart_stats_h = model_h;
        m_kart_stats_x = x + w/2 - (int)(bestSize/2);
        m_kart_stats_y = model_y + model_h;
    }
    else
    {
        const int modelMaxHeight = h - kart_name_h - player_name_h;
        const int modelMaxWidth =  w;
        const int bestSize = std::min(modelMaxWidth, modelMaxHeight);
        const int modelY = y + player_name_h;
        model_x = x + w/4 - (int)(bestSize/2);
        model_y = modelY + modelMaxHeight/2 - bestSize/2;
        model_w = bestSize;
        model_h = bestSize;

        m_kart_stats_w = w/2;
        m_kart_stats_h = h;
        m_kart_stats_x = x + w/2;
        m_kart_stats_y = y;
    }

    kart_name_x = x;
    kart_name_y = y + h - kart_name_h;
}   // setSize

// -------------------------------------------------------------------------

/** Sets which kart was selected for this player */
void PlayerKartWidget::setKartInternalName(const std::string& whichKart)
{
    assert(m_magic_number == 0x33445566);
    m_kartInternalName = whichKart;
}   // setKartInternalName

// -------------------------------------------------------------------------

const std::string& PlayerKartWidget::getKartInternalName() const
{
    assert(m_magic_number == 0x33445566);
    return m_kartInternalName;
}   // getKartInternalName

// -------------------------------------------------------------------------

/** \brief Event callback from ISpinnerConfirmListener */
EventPropagation PlayerKartWidget::onSpinnerConfirmed()
{
    KartSelectionScreen::getRunningInstance()->playerConfirm(m_player_id);
    return EVENT_BLOCK;
}   // onSpinnerConfirmed
